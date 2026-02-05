import "dotenv/config";

// Change this if we want to scrape the past
export const YEAR = process.env.YEAR
  ? parseInt(process.env.YEAR)
  : new Date().getFullYear();
console.log("Scraping " + YEAR);

export const HASURAGRES_URL = process.env.HASURAGRES_URL;
console.log("Url to hit: " + HASURAGRES_URL);

export const HASURAGRES_API_KEY = process.env.HASURAGRES_API_KEY;

export const MAX_CONCURRENT_REQUESTS = process.env.MAX_CONCURRENT_REQUESTS
  ? parseInt(process.env.MAX_CONCURRENT_REQUESTS)
  : 1;
console.log("Maximum Concurrent Requests: " + MAX_CONCURRENT_REQUESTS);

export const MIN_TIME_BETWEEN_REQUESTS = process.env
  .MIN_TIME_MS_BETWEEN_REQUESTS
  ? parseInt(process.env.MIN_TIME_MS_BETWEEN_REQUESTS)
  : 5000;
console.log("Minimum Time Between Requests: " + MIN_TIME_BETWEEN_REQUESTS);

export const DRYRUN = !!process.env.DRYRUN;
