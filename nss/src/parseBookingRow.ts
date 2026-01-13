import { formatInTimeZone, zonedTimeToUtc } from "date-fns-tz";
import PARSERS from "./nameParsers";
import parseRoomIds from "./parseRoomIds";
import { BookingsExcelRow, ParsedName, RoomBooking } from "./types";

const DAYS = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];

export function parseBookingRow(booking: BookingsExcelRow): RoomBooking[] {
  // console.log("BookingExcelRow input: ");
  // console.log(booking);

  const bookings: RoomBooking[] = [];
  // TODO: uncomment when done
  // const { bookingType, name } = parseName(booking.name);
  const roomIds: string[] = parseRoomIds(booking.allocated_location_name);
  const bookingDates = parseDateRanges(booking.dates, booking.day, booking.start_time);

  // console.log("Parsed dates:", bookingDates);

  for (const date of bookingDates) {
    const start = date;

    // Build end time
    const dateStr = formatInTimeZone(date, 'Australia/Sydney', 'yyyy-MM-dd');
    const endString = `${dateStr}T${booking.end_time}:00`;
    const end = zonedTimeToUtc(endString, 'Australia/Sydney');

    for (const roomId of roomIds) {
      bookings.push({
        bookingType: '', // todo: remove placeholder once combined with yanlin's name parsers
        name: '', // todo: remove placeholder once combined with yanlin's name parsers
        roomId,
        start,
        end
      });
    }
  }

  // console.log("RoomBooking output: ");
  // console.log(bookings);
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

/**
 * Takes in a date range string
 * (ex. "16/02/2026 - 16/03/2026 \n30/03/2026 \n13/04/2026 - 20/04/2026"),
 * day of the week (ex. "Monday"), and start time (ex. "06:00").
 * Returns Date objects of the specified day and start time within the ranges
 * ex. Date object of all Mondays at 06:00 in the ranges
 */
const parseDateRanges = (dateRangeString: string, dayOfWeek: string, startTime: string): Date[] => {
  const dates: Date[] = [];
  const targetDay = DAYS.indexOf(dayOfWeek);
  const ranges = dateRangeString.split('\n').map(s => s.trim()).filter(Boolean);

  for (const range of ranges) {
    if (range.includes(' - ')) {
      // Date range (ex. "16/02/2026 - 16/03/2026")
      const [startStr, endStr] = range.split(' - ').map(s => s.trim());
      const startDate = zonedTimeToUtc(`${formatDateSubstring(startStr)}T${startTime}:00`, 'Australia/Sydney');
      const endDate = zonedTimeToUtc(`${formatDateSubstring(endStr)}T${startTime}:00`, 'Australia/Sydney');

      // Find the first occurrence of target day in the range
      // Need to check the day in sydney time
      let current = new Date(startDate);
      while (DAYS.indexOf(formatInTimeZone(current, 'Australia/Sydney', 'EEEE')) !== targetDay && current <= endDate) {
        current.setDate(current.getDate() + 1);
      }

      // Jump by 7 days to get all occurrences in the range
      while (current <= endDate) {
        dates.push(new Date(current));
        current.setDate(current.getDate() + 7);
      }
    } else {
      // Single date (ex. "30/03/2026")
      const date = zonedTimeToUtc(`${formatDateSubstring(range)}T${startTime}:00`, 'Australia/Sydney');
      dates.push(date);
    }
  }

  return dates;
}

/**
 * Convert "DD/MM/YYYY" to "YYYY-MM-DD"
 */
const formatDateSubstring = (dateStr: string): string => {
  const [day, month, year] = dateStr.split('/');
  return `${year}-${month}-${day}`;
}
