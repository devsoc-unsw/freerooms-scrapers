import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import scrapeBookings from "./scrapeBookings";
import parseBooking from "./parseBooking";
import scrapeBuildings from "./scrapeBuildings";

const runScrapeJob = async () => {

  const buildings = await scrapeBuildings();
  fs.writeFileSync("./buildings.json", JSON.stringify(buildings, null, 2));

  const rooms = await scrapeRooms();
  fs.writeFileSync("./rooms.json", JSON.stringify(rooms, null, 2));

  const bookingPromises = rooms.map(room => scrapeBookings(room.id));
  const bookings = (await Promise.all(bookingPromises)).flat();
  const parsedBookings = bookings.map(parseBooking).flat();
  parsedBookings.sort((a, b) => a.start.getTime() - b.start.getTime());
  fs.writeFileSync("./bookings.json", JSON.stringify(parsedBookings));
}

console.time('Scraping');
runScrapeJob().then(() => console.timeEnd('Scraping'));
