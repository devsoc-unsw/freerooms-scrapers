import axios from "axios";
import fs from "fs";
import scrapeBookings from "./bookings/scrapeBookings";
import { DRYRUN, HASURAGRES_API_KEY, HASURAGRES_URL, YEAR } from "./config";
import scrapeBuildings from "./scrapeBuildings";
import { scrapeRoomFacilities } from "./scrapeRoomFacilities";
import scrapeRooms from "./scrapeRooms";
import { formatString } from "./stringUtils";

const runScrapeJob = async () => {
  const buildings = await scrapeBuildings();
  const rooms = await scrapeRooms();
  const facilitiesPromises = rooms.map((room) => scrapeRoomFacilities(room.id));

  // Filter buildings with no rooms
  const filteredBuildings = buildings.filter(
    (building) => !!rooms.find((room) => room.id.startsWith(building.id))
  );

  // we're sending about 1000 requests here
  const facilities = await Promise.all(facilitiesPromises);
  const bookings = await scrapeBookings();
  bookings.sort((a, b) => a.start.getTime() - b.start.getTime());

  return {
    buildings: filteredBuildings,
    rooms,
    facilities,
    bookings,
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
