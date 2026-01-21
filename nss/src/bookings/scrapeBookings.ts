import collectAllSessions from "../collectSessionIdentities";
import { YEAR } from "../config";
import { RoomBooking, RoomSessionIdentity } from "../types";
import decodeXlsx from "./decodeXlsx";
import fetchXlsx from "./fetchXlsx";
import { parseBookingRow } from "./parseBookingRow";

const TT_URL =
  "https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__";
// Specifies bookings for Mon - Sun and 06:00 - 23:30
const OPTIONS = "&view=week&days=0,1,2,3,4,5,6&timePeriod=all%20day";
// ! Can only select at most 20 at a time ! do not set higher than 20
const BATCH_SIZE = 20;
// Specify weeks and year
const TIME_PERIOD = `&datePeriod=${YEAR}&all_weeks=true`;

const generateURLs = async (): Promise<string[]> => {
  // Either read from file or fetch and save to disk
  const sessionIdentities: RoomSessionIdentity[] = await collectAllSessions();

  const urlCount = Math.ceil(sessionIdentities.length / 20);

  // Generate all batches of session ids for each rooms in groups of 20
  let batches: string[] = [];
  for (let i = 0; i < urlCount; i++) {
    const batch = sessionIdentities
      .slice(i * BATCH_SIZE, (i + 1) * BATCH_SIZE)
      .map(({ sessionIdentity }) => sessionIdentity)
      .join("_");
    batches.push(batch);
  }

  // Combine into urls
  return Promise.resolve(
    batches.map((batch) => TT_URL + batch + OPTIONS + TIME_PERIOD)
  );
};

export const scrapeBookings = async (): Promise<RoomBooking[]> => {
  return generateURLs()
    .then((urls) => urls.map(getBookings))
    .then((promisedBookings) => Promise.all(promisedBookings))
    .then((bookings) => bookings.flat());
};

const getBookings = async (url: string): Promise<RoomBooking[]> => {
  return fetchXlsx(url)
    .then(decodeXlsx)
    .then((bookingExcelRows) => bookingExcelRows.map(parseBookingRow))
    .then((nestedBookings) => nestedBookings.flat());
};
