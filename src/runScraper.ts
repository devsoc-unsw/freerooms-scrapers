import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import scrapeBookings from "./scrapeBookings";
import parseBooking from "./parseBooking";
import scrapeBuildings from "./scrapeBuildings";

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

  // Write to file
  fs.writeFileSync("./output/buildings.json", JSON.stringify(filteredBuildings, null, 2));
  fs.writeFileSync("./output/rooms.json", JSON.stringify(rooms, null, 2));
  fs.writeFileSync("./output/bookings.json", JSON.stringify(parsedBookings, null, 2));
}

console.time('Scraping');
runScrapeJob().then(() => console.timeEnd('Scraping'));
