import { ParsedName, RemoteBooking, RoomBooking } from "../types";
import PARSERS from "./nameParsers";

/***
 * Takes in a RemoteBooking and returns a list of RoomBooking(s).
 * A single BookingExcelRow has a string of room ids in allocated_location_name
 * (ex. "K-E12-205 - UNSW Business School  205, K-F23-307 - Mathews 307, K-F23-301 - Mathews 301"),
 * so multiple RoomBooking(s) are created.
 */
export function parseRemoteBooking(booking: RemoteBooking): RoomBooking[] {
  // Parse booking fields
  const bookings: RoomBooking[] = [];
  const { bookingType, name } = parseName(booking.name);
  const roomIds: string[] = parseRoomIds(booking.allocatedLocationName);
  const start = new Date(booking.startTime);
  const end = new Date(booking.endTime);

  // Create a booking for each room id
  for (const roomId of roomIds) {
    bookings.push({
      bookingType,
      name,
      roomId,
      start,
      end,
    });
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

const REGEX_ROOM_IDS = /K-[A-Z][0-9]{2}-[^ ]*/g;
// Parses the column allocated_location_name for all room ids
// Examples:
//  K-F23-226 - Mathews 226, K-F23-227 - Mathews 227, K-F23-228 - Mathews 228
//  K-B16-LG05 - Colombo Theatre C
//  K-E15-1048 - Quadrangle 1048, K-E15-1047 - Quadrangle 1047
const parseRoomIds = (allocated_location_name: string): string[] => {
  return Array.from(allocated_location_name.matchAll(REGEX_ROOM_IDS)).map(
    (match) => match[0],
  );
};
