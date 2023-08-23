import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import scrapeBookings from "./scrapeBookings";
import parseBooking from "./parseBooking";
import scrapeBuildings from "./scrapeBuildings";
import { HASURAGRES_URL } from "./config";

const runScrapeJob = async () => {

  const buildings = await scrapeBuildings();

  fetch(`${HASURAGRES_URL}/insert`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      metadata: {
        table_name: "Buildings",
        sql_create: fs.readFileSync("./sql/Buildings.sql", "utf8"),
        columns: ["id", "name", "lat", "long"],
      },
      payload: buildings
    })
  });

  const rooms = await scrapeRooms();
  fetch(`${HASURAGRES_URL}/insert`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      metadata: {
        table_name: "Rooms",
        columns: ["abbr", "name", "id", "usage", "capacity", "school", "buildingid"],
        sql_create: fs.readFileSync("./sql/Rooms.sql", "utf8")
      },
      payload: rooms
    })
  });

  const bookingPromises = rooms.map(room => scrapeBookings(room.id));
  const bookings = (await Promise.all(bookingPromises)).flat();
  const parsedBookings = bookings.map(parseBooking).flat();
  parsedBookings.sort((a, b) => a.start.getTime() - b.start.getTime());
  fetch(`${HASURAGRES_URL}/insert`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      metadata: {
        table_name: "RoomBookings",
        columns: ["bookingType", "name", "id", "roomId", "start", "end"],
        sql_create: fs.readFileSync("./sql/RoomBookings.sql", "utf8")
      },
      payload: parsedBookings
    })
  });
}

console.time('Scraping');
runScrapeJob().then(() => console.timeEnd('Scraping'));
