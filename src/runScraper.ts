import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import scrapeBookings from "./scrapeBookings";
import parseBooking from "./parseBooking";
import scrapeBuildings from "./scrapeBuildings";
import { HASURAGRES_URL } from "./config";
import axios from 'axios';

const runScrapeJob = async () => {
  const buildings = await scrapeBuildings();
  const rooms = await scrapeRooms();

  // Filter buildings with no rooms
  const filteredBuildings = buildings.filter(
    building => !!rooms.find(room => room.id.startsWith(building.id))
  );

  const bookingPromises = rooms.map(room => scrapeBookings(room.id));
  const bookings = (await Promise.all(bookingPromises)).flat();
  const parsedBookings = bookings.map(parseBooking).flat();
  parsedBookings.sort((a, b) => a.start.getTime() - b.start.getTime());

  await axios.post(
    `${HASURAGRES_URL}/insert`,
    {
      metadata: {
        table_name: "Buildings",
        sql_up: fs.readFileSync("./sql/buildings/up.sql", "utf8"),
        sql_down: fs.readFileSync("./sql/buildings/down.sql", "utf8"),
        columns: ["id", "name", "lat", "long", "aliases"],
      },
      payload: filteredBuildings
    },
    {
      headers: {
        "Content-Type": "application/json"
      }
    }
  );

  await axios.post(
    `${HASURAGRES_URL}/insert`,
    {
      metadata: {
        table_name: "Rooms",
        columns: ["abbr", "name", "id", "usage", "capacity", "school", "buildingId"],
        sql_up: fs.readFileSync("./sql/rooms/up.sql", "utf8"),
        sql_down: fs.readFileSync("./sql/rooms/down.sql", "utf8"),
      },
      payload: rooms
    },
    {
      headers: {
        "Content-Type": "application/json"
      }
    }
  );

  await axios.post(
    `${HASURAGRES_URL}/insert`,
    {
      metadata: {
        table_name: "Bookings",
        columns: ["bookingType", "name", "roomId", "start", "end"],
        sql_up: fs.readFileSync("./sql/bookings/up.sql", "utf8"),
        sql_down: fs.readFileSync("./sql/bookings/down.sql", "utf8"),
      },
      payload: parsedBookings
    },
    {
      headers: {
        "Content-Type": "application/json"
      }
    }
  );

}

console.time('Scraping');
runScrapeJob().then(() => console.timeEnd('Scraping'));
