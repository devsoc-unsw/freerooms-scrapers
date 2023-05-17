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

      // Determine which events to extends and which to mark as ended
      const newOpenEvents: UngroupedRoomBooking[] = [];
      const currEventSet = new Set(currEvents.map(e => e.name));
      for (const openEvent of openEvents[day]) {
        if (currEventSet.has(openEvent.name)) {
          // If matching event in this time slot, extend it
          currEventSet.delete(openEvent.name);
          newOpenEvents.push(openEvent);
        } else {
          // Otherwise mark the event as ended
          bookings.push({ ...openEvent, roomId: roomId, end: time });
        }
      }

      // Curr events left in the set are new and should be added to open
      for (const currEvent of currEvents) {
        if (currEventSet.has(currEvent.name)) {
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

export default scrapeBookings;
