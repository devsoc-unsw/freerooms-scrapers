import axios from "axios";
import fs from "fs";
import scrapeBookings from "./bookings/scrapeBookings";
import { DRYRUN, HASURAGRES_API_KEY, HASURAGRES_URL, YEAR } from "./config";
import { formatString } from "./stringUtils";
import { NSS_DATA_PATH } from "./constants";
import path from "path";
import { Building, MappedFacilities, Room } from "./types";

const runScrapeJob = async () => {
  const buildings = JSON.parse(fs.readFileSync(path.join(NSS_DATA_PATH, "buildings.json"), 'utf8')) as Building[];
  const rooms = JSON.parse(fs.readFileSync(path.join(NSS_DATA_PATH, "rooms.json"), 'utf8')) as Room[];
  const facilities = JSON.parse(fs.readFileSync(path.join(NSS_DATA_PATH, "facilities.json"), 'utf8')) as MappedFacilities[];

  // Filter buildings with no rooms
  const filteredBuildings = buildings.filter(
    (building) => !!rooms.find((room) => room.id.startsWith(building.id))
  );

  const bookings = await scrapeBookings();
  bookings.sort((a, b) => a.start.getTime() - b.start.getTime());
  // Ensures any bookings are only for rooms we have fetched
  const filteredBookings = bookings.filter(booking => rooms.map(room => room.id).includes(booking.roomId))

  return {
    buildings: filteredBuildings,
    rooms,
    facilities,
    bookings: filteredBookings,
  };
};

const runScraper = async () => {
  console.time("Scraping");
  const { buildings, rooms, facilities, bookings } = await runScrapeJob();
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
          sql_up: fs.readFileSync("./sql/buildings/up.sql", "utf8"),
          sql_down: fs.readFileSync("./sql/buildings/down.sql", "utf8"),
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
          ],
          sql_up: fs.readFileSync("./sql/rooms/up.sql", "utf8"),
          sql_down: fs.readFileSync("./sql/rooms/down.sql", "utf8"),
          sql_before: formatString(
            fs.readFileSync("./sql/rooms/before.sql", "utf8"),
            rooms.map((room) => `'${room.id}'`).join(",")
          ),
          write_mode: "append",
          dryrun: DRYRUN,
        },
        payload: rooms.map((room, i) => ({
          ...room,
          ...facilities[i],
        })),
      },
      {
        metadata: {
          table_name: "Bookings",
          columns: ["bookingType", "name", "roomId", "start", "end"],
          sql_up: fs.readFileSync("./sql/bookings/up.sql", "utf8"),
          sql_down: fs.readFileSync("./sql/bookings/down.sql", "utf8"),
          sql_before: formatString(
            fs.readFileSync("./sql/bookings/before.sql", "utf8"),
            new Date(YEAR, 0, 1).toISOString(),
            new Date(YEAR + 1, 0, 1).toISOString()
          ),
          write_mode: "append",
          dryrun: DRYRUN,
        },
        payload: bookings,
      },
    ],
    requestConfig
  );
  console.timeEnd("Inserting");
};

runScraper();
