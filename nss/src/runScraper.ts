import axios from "axios";
import fs from "fs";
import scrapeBookings from "./bookings/scrapeBookings";
import { DRYRUN, HASURAGRES_API_KEY, HASURAGRES_URL, YEAR } from "./config";
import { formatString } from "./stringUtils";
import { NSS_DATA_PATH } from "./constants";
import path from "path";
import { Building, MappedFacilities, Room } from "./types";

const readSql = (sqlPath: string): string => {
  const candidatePaths = [
    path.resolve(__dirname, "..", sqlPath),
    path.resolve(__dirname, "..", "..", sqlPath),
  ];

  const resolvedPath = candidatePaths.find((p) => fs.existsSync(p));
  if (!resolvedPath) {
    throw new Error(`SQL file not found: ${sqlPath} (tried ${candidatePaths.join(", ")})`);
  }

  let content = fs.readFileSync(resolvedPath, "utf8").trim();
  if (/^(\.\.\/)+sql\//.test(content)) {
    content = fs.readFileSync(
      path.resolve(path.dirname(resolvedPath), content),
      "utf8",
    );
  }
  return content;
}

const runScrapeJob = async () => {
  const buildings = JSON.parse(
    fs.readFileSync(path.join(NSS_DATA_PATH, "buildings.json"), "utf8"),
  ) as Building[];
  const rooms = JSON.parse(
    fs.readFileSync(path.join(NSS_DATA_PATH, "rooms.json"), "utf8"),
  ) as Room[];
  const facilities = JSON.parse(
    fs.readFileSync(path.join(NSS_DATA_PATH, "facilities.json"), "utf8"),
  ) as MappedFacilities[];

  let roomEmbeddings: { id: string; embedding: number[] }[] = [];
  try {
    roomEmbeddings = JSON.parse(
      fs.readFileSync(path.join(NSS_DATA_PATH, "room_embeddings.json"), "utf8"),
    ) as { id: string; embedding: number[] }[];
  } catch {
    // room_embeddings.json doesn't exist yet; that's fine (embeddings will be NULL)
  }
  const roomEmbeddingsMap = new Map(roomEmbeddings.map((e) => [e.id, e.embedding]));

  // Filter buildings with no rooms
  const filteredBuildings = buildings.filter(
    (building) => !!rooms.find((room) => room.id.startsWith(building.id)),
  );

  const bookings = await scrapeBookings();
  bookings.sort((a, b) => a.start.getTime() - b.start.getTime());
  // Ensures any bookings are only for rooms we have fetched
  const filteredBookings = bookings.filter((booking) =>
    rooms.map((room) => room.id).includes(booking.roomId),
  );

  return {
    buildings: filteredBuildings,
    rooms,
    facilities,
    bookings: filteredBookings,
    roomEmbeddingsMap,
  };
};

const runScraper = async () => {
  console.time("Scraping");
  const { buildings, rooms, facilities, bookings, roomEmbeddingsMap } =
    await runScrapeJob();
  console.timeEnd("Scraping");

  const requestConfig = {
    headers: {
      "Content-Type": "application/json",
      "X-Api-Key": HASURAGRES_API_KEY,
    },
  };

  console.time("Inserting");
  await axios.post(
    `${HASURAGRES_URL}/batch_insert`,
    [
      {
        metadata: {
          table_name: "Buildings",
          sql_up: readSql("sql/buildings/up.sql"),
          sql_down: readSql("sql/buildings/down.sql"),
          columns: ["id", "name", "lat", "long", "aliases"],
          write_mode: "overwrite",
          dryrun: DRYRUN,
        },
        payload: buildings,
      },
      {
        metadata: {
          table_name: "Rooms",
          columns: [
            "abbr",
            "name",
            "id",
            "usage",
            "capacity",
            "school",
            "buildingId",
            "floor",
            "seating",
            "microphone",
            "accessibility",
            "audiovisual",
            "infotechnology",
            "writingMedia",
            "service",
            "lat",
            "long",
            "embedding",
          ],
          sql_up: readSql("sql/rooms/up.sql"),
          sql_down: readSql("sql/rooms/down.sql"),
          sql_before: formatString(
            readSql("sql/rooms/before.sql"),
            rooms.map((room) => `'${room.id}'`).join(","),
          ),
          write_mode: "append",
          dryrun: DRYRUN,
        },
        payload: rooms.map((room, i) => ({
          ...room,
          ...facilities[i],
          embedding: roomEmbeddingsMap.get(room.id) ?? null,
        })),
      },
      {
        metadata: {
          table_name: "Bookings",
          columns: ["bookingType", "name", "roomId", "start", "end"],
          sql_up: readSql("sql/bookings/up.sql"),
          sql_down: readSql("sql/bookings/down.sql"),
          sql_before: formatString(
            readSql("sql/bookings/before.sql"),
            new Date(YEAR, 0, 1).toISOString(),
            new Date(YEAR + 1, 0, 1).toISOString(),
          ),
          write_mode: "append",
          dryrun: DRYRUN,
        },
        payload: bookings,
      },
    ],
    requestConfig,
  );
  console.timeEnd("Inserting");
};

runScraper();
