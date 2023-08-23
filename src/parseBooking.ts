import PARSERS from "./nameParsers";
import { ParsedName, RawRoomBooking, RoomBooking } from "./types";
import toSydneyTime from "./toSydneyTime";
const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
const NO_WEEKS = 52;
const FIRST_WEEK = 0x8000000000000n;

/**
 * Takes a raw room booking and:
 * - parses the name into a type and human-readable name
 * - splits the booking's week pattern into a list of bookings with
 *   start and end Dates
 */
function parseBooking(booking: RawRoomBooking) {
  const { bookingType, name } = parseName(booking.name);

  const bookings: RoomBooking[] = [];
  // The weekPattern is a 52 bit integer - if the first bit (from left) is set
  // this means the booking runs in the first week etc.
  const weekPattern = BigInt(booking.weekPattern);
  let weekMask = FIRST_WEEK;
  for (let i = 0; i < NO_WEEKS; i++) {
    if (weekPattern & weekMask) {
      bookings.push({
        bookingType: bookingType,
        name,
        roomId: booking.roomId,
        start: toSydneyTime(createDate(i, booking.day, booking.start)),
        end: toSydneyTime(createDate(i, booking.day, booking.end)),
      });
    }
    weekMask >>= 1n;
  }
  return bookings;
}

const parseName = (rawName: string): ParsedName => {
  // Try all the parsers
  for (const parser of Object.values(PARSERS)) {
    const match = rawName.match(parser.pattern);
    if (match && match.groups) {
      return parser.parser(match.groups);
    }
  }

  console.warn(`Warning: No pattern found for "${rawName}"`);
  return { bookingType: 'MISC',  name: 'Misc.' }
}

/**
 * Create a date given a week number (0..52), day (full name) and time (HH:MM)
 */
const createDate = (week: number, day: string, time: string) => {
  const dayNum = DAYS.indexOf(day);
  const [hours, minutes] = time.split(':').map(x => parseInt(x, 10));
  const year = new Date().getFullYear();

  // Find the date of the first monday
  const firstDay = new Date(year, 0, 1);
  const firstMonday = 1 + ((8 - firstDay.getDay()) % 7);

  // Add weeks and day - if the day is greater than the number of days in a
  // month then JS just overflows it to the next
  return new Date(year, 0, firstMonday + week * 7 + dayNum, hours, minutes);
}

export default parseBooking;
