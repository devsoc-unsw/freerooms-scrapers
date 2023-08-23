import 'dotenv/config'

export const HASURAGRES_URL = `http://${process.env.HASURAGRES_HOST}:${process.env.HASURAGRES_PORT}`;
console.log("Url to hit: " + HASURAGRES_URL);
