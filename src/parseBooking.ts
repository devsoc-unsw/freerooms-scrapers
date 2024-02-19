import PARSERS from "./nameParsers";
import { ParsedName, RawRoomBooking, RoomBooking } from "./types";
import { toSydneyTime, createDate, numWeeksInYear } from "./dateUtils";

/**
 * Takes a raw room booking and:
 * - parses the name into a type and human-readable name
 * - splits the booking's week pattern into a list of bookings with
 *   start and end Dates
 */
function parseBooking(booking: RawRoomBooking): RoomBooking[] {
  const { bookingType, name } = parseName(booking.name);

  const bookings: RoomBooking[] = [];
  // The weekPattern is a 52 (or 53) bit integer - if the first bit (from left)
  // is set this means the booking runs in the first week etc.
  const weekPattern = BigInt(booking.weekPattern);
  const numWeeks = numWeeksInYear(new Date().getFullYear());
  let weekMask = 1n << BigInt(numWeeks - 1);
  for (let i = 0; i < numWeeks; i++) {
    if (weekPattern & weekMask) {
      const start = toSydneyTime(createDate(i, booking.day, booking.start));
      const end = toSydneyTime(createDate(i, booking.day, booking.end));
      bookings.push({
        bookingType: bookingType,
        name,
        roomId: booking.roomId,
        start,
        end
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

  console.warn(`Warning: No pattern found to parse booking name "${rawName}"`);
  return { bookingType: 'MISC',  name: 'Misc.' }
}

export default parseBooking;
