import fs from "fs";
import axios from "axios";
import { DRYRUN, HASURAGRES_API_KEY, HASURAGRES_URL, YEAR } from "./config";
import decodeXlsx from "./decodeXlsx";
import fetchXlsx from "./fetchXlsx";
import { parseBookingRow } from "./parseBookingRow";
import { RoomBooking, RoomSessionIdentity } from "./types";
import { formatString } from "./stringUtils"

const TT_URL =
  "https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__";
// Specifies bookings for Mon - Sun and 06:00 - 23:30
const OPTIONS = "&view=week&days=0,1,2,3,4,5,6&timePeriod=all%20day";
const BATCH_SIZE = 20;
const TIME_PERIOD = `&datePeriod=${YEAR}&all_weeks=true`;

const scrapeTTRooms = async (): Promise<RoomBooking[]> => {
  const sessionIdentitiesRaw = fs.readFileSync(
    "./sessionIdentities.json",
    "utf8"
  );
  const sessionIdentities: RoomSessionIdentity[] =
    JSON.parse(sessionIdentitiesRaw);

  const bookings = [];
  // Process batches of 20 rooms
  for (let i = 0; i < sessionIdentities.length; i += BATCH_SIZE) {
    const batch = sessionIdentities.slice(i, i + BATCH_SIZE);
    const url =
      TT_URL +
      batch
        .map((s: { id: string; sessionIdentity: string }) => s.sessionIdentity)
        .join("_") +
      OPTIONS +
      TIME_PERIOD;

    console.log(url);
    console.log(`Processing batch ${i} to ${i + BATCH_SIZE}...`);

    try {
      bookings.push(getBookings(url, i));
    } catch (error) {
      console.log(`Continuing with next batch...`);
    }
  }

  return Promise.all(bookings)
    .then((bookings) => bookings.flat())
    .then((bookings) => {
      // TODO: Only here for debugging purposes remove when in prod
      fs.writeFileSync("bookings.json", JSON.stringify(bookings, null, 2));
      return bookings;
    });
};

const getBookings = async (url: string, i: number): Promise<RoomBooking[]> => {
  const bookings = [];

  if (await fetchXlsx(url, `./${i}`)) {
    const ttBookings = decodeXlsx(`./${i}.xlsx`); // This returns excel booking row type
    // Call your function here to test and run 'npm run scrape-publish'
    for (let i = 0; i < ttBookings.length; i++) {
      bookings.push(parseBookingRow(ttBookings[i]));
    }
  }

  return bookings.flat();
};

function toApiBooking(b: any) {
  return {
    bookingType: b.bookingType ?? "timetable",
    name: b.name ?? b.activity ?? b.title ?? b.eventName ?? "",
    roomId: b.roomId ?? b.room_id ?? b.room ?? "",
    start: b.start,
    end: b.end,
  };
}

async function run() {
  console.time("Scrape TT bookings");
  const bookings = await scrapeTTRooms();
  console.timeEnd("Scrape TT bookings");

  const payload = bookings.map(toApiBooking);

  const requestConfig = {
    headers: {
      "Content-Type": "application/json",
      "X-Api-Key": HASURAGRES_API_KEY,
    },
  };

  console.time("Upload bookings");
  await axios.post(
    `${HASURAGRES_URL}/batch_insert`,
    [
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
        payload,
      },
    ],
    requestConfig
  );
  console.timeEnd("Upload bookings");

  console.log(`Uploaded ${payload.length} bookings (dryrun=${DRYRUN})`);
}

run();
