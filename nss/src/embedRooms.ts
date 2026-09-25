import "dotenv/config";
import { GoogleGenAI } from "@google/genai";
import fs from "fs";
import path from "path";
import { Room, MappedFacilities } from "./types";
import { NSS_DATA_PATH } from "./constants";
import { GEMINI_API_KEY } from "./config";

const CHUNK_SIZE = 100;
const INTER_CHUNK_DELAY_MS = 61_000;
const MAX_RETRIES = 5;

type RoomEmbedding = { id: string; embedding: number[] };

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

// gemini-embedding-001 only auto-normalizes 3072-dim output; truncated
// dimensions (e.g. 768) must be normalized manually for cosine similarity.
const normalize = (values: number[]): number[] => {
  const norm = Math.sqrt(values.reduce((sum, v) => sum + v * v, 0));
  return norm === 0 ? values : values.map((v) => v / norm);
};

const buildRoomBody = (room: Room, facilities: MappedFacilities): string => {
  const parts = [
    `${room.name} (${room.abbr}) — ${room.usage}, capacity ${room.capacity}.`,
    facilities.floor ? `Floor: ${facilities.floor}.` : null,
    facilities.seating ? `Seating: ${facilities.seating}.` : null,
    facilities.audiovisual.length
      ? `Audio-visual: ${facilities.audiovisual.join(", ")}.`
      : null,
    facilities.infotechnology.length
      ? `Info technology: ${facilities.infotechnology.join(", ")}.`
      : null,
    facilities.writingMedia.length
      ? `Writing media: ${facilities.writingMedia.join(", ")}.`
      : null,
    facilities.accessibility.length
      ? `Accessibility: ${facilities.accessibility.join(", ")}.`
      : null,
    facilities.microphone.length
      ? `Microphone: ${facilities.microphone.join(", ")}.`
      : null,
    facilities.service.length
      ? `Services: ${facilities.service.join(", ")}.`
      : null,
  ];
  return parts.filter(Boolean).join(" ");
};

const parseRetryDelayMs = (err: unknown): number => {
  const message = err instanceof Error ? err.message : String(err);
  const match = message.match(/retry in (\d+(?:\.\d+)?)s/i);
  if (match) {
    return Math.ceil(parseFloat(match[1]) * 1000) + 500;
  }
  return INTER_CHUNK_DELAY_MS;
};

const embedChunkWithRetry = async (ai: GoogleGenAI, chunk: string[]) => {
  for (let attempt = 1; attempt <= MAX_RETRIES; attempt++) {
    try {
      return await ai.models.embedContent({
        model: "gemini-embedding-001",
        contents: chunk,
        config: {
          outputDimensionality: 768,
          taskType: "RETRIEVAL_DOCUMENT",
        },
      });
    } catch (err: unknown) {
      const status = (err as { status?: number }).status;
      if (status === 429 && attempt < MAX_RETRIES) {
        const delayMs = parseRetryDelayMs(err);
        console.warn(
          `Rate limited (attempt ${attempt}/${MAX_RETRIES}), retrying in ${Math.round(delayMs / 1000)}s...`,
        );
        await sleep(delayMs);
        continue;
      }
      throw err;
    }
  }
  throw new Error("Failed to embed chunk after max retries");
};

const saveEmbeddings = (
  embeddingsPath: string,
  results: RoomEmbedding[],
) => {
  fs.writeFileSync(embeddingsPath, JSON.stringify(results, null, 2));
};

const embedRooms = async () => {
  const embeddingsPath = path.join(NSS_DATA_PATH, "room_embeddings.json");
  const force = process.argv.includes("--force");

  let existing: RoomEmbedding[] = [];
  if (!force) {
    try {
      existing = JSON.parse(fs.readFileSync(embeddingsPath, "utf8"));
    } catch {
      // file doesn't exist yet, that's fine
    }
  }

  // Fix up embeddings saved before normalization was added.
  if (existing.length > 0) {
    const firstNorm = Math.sqrt(
      existing[0].embedding.reduce((sum, v) => sum + v * v, 0),
    );
    if (Math.abs(firstNorm - 1) > 1e-6) {
      console.log(`Normalizing ${existing.length} existing embeddings...`);
      existing = existing.map((e) => ({
        ...e,
        embedding: normalize(e.embedding),
      }));
      saveEmbeddings(embeddingsPath, existing);
    }
  }

  const rooms = JSON.parse(
    fs.readFileSync(path.join(NSS_DATA_PATH, "rooms.json"), "utf8"),
  ) as Room[];
  const facilities = JSON.parse(
    fs.readFileSync(path.join(NSS_DATA_PATH, "facilities.json"), "utf8"),
  ) as MappedFacilities[];

  if (rooms.length !== facilities.length) {
    throw new Error(
      `rooms.json (${rooms.length}) and facilities.json (${facilities.length}) are out of sync`,
    );
  }

  if (!force && existing.length >= rooms.length) {
    console.log(
      `room_embeddings.json already has ${existing.length} entries. Pass --force to overwrite.`,
    );
    process.exit(0);
  }

  if (!force && existing.length > 0) {
    console.log(
      `Resuming from ${existing.length}/${rooms.length} existing embeddings...`,
    );
  }

  if (!GEMINI_API_KEY) {
    throw new Error(
      "GEMINI_API_KEY is not set. A gemini api key is required to embed rooms.",
    );
  }

  const ai = new GoogleGenAI({ apiKey: GEMINI_API_KEY });
  const bodies = rooms.map((room, i) => buildRoomBody(room, facilities[i]));
  const existingIds = new Set(existing.map((e) => e.id));
  const results: RoomEmbedding[] = force ? [] : [...existing];

  const pendingIndexes = rooms
    .map((_, i) => i)
    .filter((i) => !existingIds.has(rooms[i].id));

  if (pendingIndexes.length === 0) {
    console.log("All rooms already embedded.");
    return;
  }

  console.log(`Embedding ${pendingIndexes.length} remaining rooms...`);

  try {
    for (let i = 0; i < pendingIndexes.length; i += CHUNK_SIZE) {
      const chunkIndexes = pendingIndexes.slice(i, i + CHUNK_SIZE);
      const chunk = chunkIndexes.map((idx) => bodies[idx]);
      const startRoom = i + 1;
      const endRoom = Math.min(i + CHUNK_SIZE, pendingIndexes.length);

      console.log(
        `Embedding rooms ${startRoom}–${endRoom} of ${pendingIndexes.length} remaining...`,
      );

      const response = await embedChunkWithRetry(ai, chunk);

      response.embeddings!.forEach((embedding, j) => {
        const roomIdx = chunkIndexes[j];
        results.push({
          id: rooms[roomIdx].id,
          embedding: normalize(embedding.values!),
        });
      });

      saveEmbeddings(embeddingsPath, results);
      console.log(`Saved ${results.length}/${rooms.length} embeddings.`);

      if (i + CHUNK_SIZE < pendingIndexes.length) {
        console.log(
          `Waiting ${INTER_CHUNK_DELAY_MS / 1000}s for free-tier rate limit...`,
        );
        await sleep(INTER_CHUNK_DELAY_MS);
      }
    }
  } catch (err) {
    saveEmbeddings(embeddingsPath, results);
    console.error(
      `Failed after saving ${results.length}/${rooms.length} embeddings. Re-run to resume.`,
    );
    throw err;
  }

  console.log(
    `Embedding complete! Wrote ${results.length} embeddings to room_embeddings.json.`,
  );
};

embedRooms();
