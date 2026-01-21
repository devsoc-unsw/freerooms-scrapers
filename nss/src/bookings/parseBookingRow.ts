import { formatInTimeZone, zonedTimeToUtc } from "date-fns-tz";
import { BookingsExcelRow, ParsedName, RoomBooking } from "../types";
import PARSERS from "../nameParsers";

const DAYS = [
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
];

export function parseBookingRow(booking: BookingsExcelRow): RoomBooking[] {
  const bookings: RoomBooking[] = [];
  const { bookingType, name } = parseName(booking.name);
  console.log(booking);
  const roomIds: string[] = parseRoomIds(booking.allocated_location_name);
  const bookingDates = parseDateRanges(
    booking.dates,
    booking.day,
    booking.start_time
  );

  for (const date of bookingDates) {
    const start = date;

    // Build end time
    const dateStr = formatInTimeZone(date, "Australia/Sydney", "yyyy-MM-dd");
    const endString = `${dateStr}T${booking.end_time}:00`;
    const end = zonedTimeToUtc(endString, "Australia/Sydney");

    for (const roomId of roomIds) {
      bookings.push({
        bookingType,
        name,
        roomId,
        start,
        end,
      });
    }
  }

  return bookings;
}

const parseName = (rawName: string): ParsedName => {
  for (const parser of Object.values(PARSERS)) {
    const match = rawName.match(parser.pattern);
    if (match && match.groups) {
      return parser.parser(match.groups);
    }
  }

  console.warn(`Warning: No pattern found to parse booking name "${rawName}"`);
  return { bookingType: "MISC", name: "Misc." };
};

/**
 * Takes in a date range string
 * (ex. "16/02/2026 - 16/03/2026 \n30/03/2026 \n13/04/2026 - 20/04/2026"),
 * day of the week (ex. "Monday"), and start time (ex. "06:00").
 * Returns Date objects of the specified day and start time within the ranges
 * ex. Date object of all Mondays at 06:00 in the ranges
 */
const parseDateRanges = (
  dateRangeString: string,
  dayOfWeek: string,
  startTime: string
): Date[] => {
  const dates: Date[] = [];
  const targetDay = DAYS.indexOf(dayOfWeek);
  const ranges = dateRangeString
    .split("\n")
    .map((s) => s.trim())
    .filter(Boolean);

  for (const range of ranges) {
    if (range.includes(" - ")) {
      // Date range (ex. "16/02/2026 - 16/03/2026")
      const [startStr, endStr] = range.split(" - ").map((s) => s.trim());
      const startDate = zonedTimeToUtc(
        `${formatDateSubstring(startStr)}T${startTime}:00`,
        "Australia/Sydney"
      );
      const endDate = zonedTimeToUtc(
        `${formatDateSubstring(endStr)}T${startTime}:00`,
        "Australia/Sydney"
      );

      // Find the first occurrence of target day in the range
      // Need to check the day in sydney time
      let current = new Date(startDate);
      while (
        DAYS.indexOf(formatInTimeZone(current, "Australia/Sydney", "EEEE")) !==
          targetDay &&
        current <= endDate
      ) {
        current.setDate(current.getDate() + 1);
      }

      // Jump by 7 days to get all occurrences in the range
      while (current <= endDate) {
        dates.push(new Date(current));
        current.setDate(current.getDate() + 7);
      }
    } else {
      // Single date (ex. "30/03/2026")
      const date = zonedTimeToUtc(
        `${formatDateSubstring(range)}T${startTime}:00`,
        "Australia/Sydney"
      );
      dates.push(date);
    }
  }

  return dates;
};

/**
 * Convert "DD/MM/YYYY" to "YYYY-MM-DD"
 */
const formatDateSubstring = (dateStr: string): string => {
  const [day, month, year] = dateStr.split("/");
  return `${year}-${month}-${day}`;
};

const REGEX_ROOM_IDS = /K-[A-Z][0-9]{2}-[^ ]*/g;
// Parses the column allocated_location_name for all room ids
// Examples:
//  K-F23-226 - Mathews 226, K-F23-227 - Mathews 227, K-F23-228 - Mathews 228
//  K-B16-LG05 - Colombo Theatre C
//  K-E15-1048 - Quadrangle 1048, K-E15-1047 - Quadrangle 1047
const parseRoomIds = (allocated_location_name: string): string[] => {
  return Array.from(allocated_location_name.matchAll(REGEX_ROOM_IDS)).map(
    (match) => match[0]
  );
};
