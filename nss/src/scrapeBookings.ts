import { RawRoomBooking, UngroupedRoomBooking } from "./types";
import { load } from "cheerio";
import nssFetch from "./nssFetch";

const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
const ALL_WEEKS = 0xfffffffffffff;
const LATEST_TIME = "22:00";

const scrapeBookings = async (roomId: string): Promise<RawRoomBooking[]> => {
  const response = await nssFetch("view_rooms", { room: roomId });
  const $ = load(response.data);

  // Array for open events in each day
  const bookings: RawRoomBooking[] = [];
  const openEvents: Record<string, UngroupedRoomBooking[]> =
    Object.fromEntries(DAYS.map(day => [day, []]));
  $("table").find("tr.rowLowlight, tr.rowHighlight").each((_, e) => {
    const data = $(e).find("td");
    const time = $(data[0]).text().trim();
    for (let i = 1; i <= 7; i++) {
      const day = DAYS[i - 1];

      // Get all events in this timeslot
      const currEvents: UngroupedRoomBooking[] = [];
      $(data[i]).find("span").each((_, e) => {
        const weekPattern = $(e).attr("title");
        currEvents.push({
          name: $(e).text().trim(),
          day: DAYS[i - 1],
          start: time,
          weekPattern: weekPattern ? parseInt(weekPattern, 2) : ALL_WEEKS,
        });
      });

      // Determine which events to extend and which to mark as ended
      const newOpenEvents: UngroupedRoomBooking[] = [];
      for (const openEvent of openEvents[day]) {
        const hasMatchingEvent = !!currEvents.find(x => bookingsEqual(x, openEvent));
        if (hasMatchingEvent) {
          // If matching event in this time slot, extend it
          newOpenEvents.push(openEvent);
        } else {
          // Otherwise mark the event as ended
          bookings.push({ ...openEvent, roomId: roomId, end: time });
        }
      }

      // Curr events that are new (not yet in open events) should be added
      for (const currEvent of currEvents) {
        if (!openEvents[day].find(x => bookingsEqual(x, currEvent))) {
          newOpenEvents.push(currEvent);
        }
      }

      openEvents[day] = newOpenEvents;
    }
  });

  // Close up any open bookings
  for (const day of DAYS) {
    for (const openEvent of openEvents[day]) {
      bookings.push({
        ...openEvent,
        roomId: roomId,
        end: LATEST_TIME,
      });
    }
  }

  return bookings;
};

const bookingsEqual = (
  a: UngroupedRoomBooking,
  b: UngroupedRoomBooking
): boolean => {
  return a.name === b.name && a.weekPattern === b.weekPattern;
}

export default scrapeBookings;
