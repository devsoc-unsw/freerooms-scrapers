const PROD = process.env.NODE_ENV === "production";

export const SCRAPER_PATH = PROD ? "./dist/scraper.js" : "./src/scraper.ts";
export const DATABASE_PATH = "./database.json";
export const BLDG_LOCATION_PATH = "./buildingLocations.json";
export const PORT = 3000;
export const HASURAGRES_URL = `http://${process.env.HASURAGRES_HOST}:${process.env.HASURAGRES_PORT}`;
console.log(HASURAGRES_URL);
