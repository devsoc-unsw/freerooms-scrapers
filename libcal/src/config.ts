import 'dotenv/config'

export const HASURAGRES_URL = process.env.HASURAGRES_URL;
console.log("Url to hit: " + HASURAGRES_URL);

export const HASURAGRES_API_KEY = process.env.HASURAGRES_API_KEY;

export const DRYRUN = !!process.env.DRYRUN;
