import "dotenv/config";
import { GoogleGenAI } from "@google/genai";
import fs from "fs";
import path from "path";
import { Room, MappedFacilities } from "./types";
import { NSS_DATA_PATH } from "./constants";
import { GEMINI_API_KEY } from "./config";

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

const embedRooms = async () => {
  const embeddingsPath = path.join(NSS_DATA_PATH, "room_embeddings.json");

  // Safety guard — refuse to overwrite unless --force is passed
  let existing = [];
  try {
    existing = JSON.parse(fs.readFileSync(embeddingsPath, "utf8"));
  } catch {
    // file doesn't exist yet, that's fine
  }
  if (existing.length > 0 && !process.argv.includes("--force")) {
    console.log(
      `room_embeddings.json already has ${existing.length} entries. Pass --force to overwrite. This will overwrite all existing embeddings`,
    );
    process.exit(0);
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

  if (!GEMINI_API_KEY) {
    throw new Error(
      "GEMINI_API_KEY is not set. A gemini api key is required to embed rooms.",
    );
  }

  const ai = new GoogleGenAI({ apiKey: GEMINI_API_KEY });

  const bodies = rooms.map((room, i) => buildRoomBody(room, facilities[i]));
  const results: { id: string; embedding: number[] }[] = [];
  const CHUNK_SIZE = 100;

  for (let i = 0; i < bodies.length; i += CHUNK_SIZE) {
    const chunk = bodies.slice(i, i + CHUNK_SIZE);
    console.log(
      `Embedding rooms ${i + 1}–${Math.min(i + CHUNK_SIZE, bodies.length)} of ${bodies.length}...`,
    );

    const response = await ai.models.embedContent({
      model: "gemini-embedding-001",
      contents: chunk,
      config: {
        outputDimensionality: 768,
        taskType: "RETRIEVAL_DOCUMENT",
      },
    });

    response.embeddings!.forEach((embedding, j) => {
      results.push({ id: rooms[i + j].id, embedding: embedding.values! });
    });
  }

  fs.writeFileSync(embeddingsPath, JSON.stringify(results, null, 2));
  console.log(
    `Embedding complete! Wrote ${results.length} embeddings to room_embeddings.json.`,
  );
};

embedRooms();
