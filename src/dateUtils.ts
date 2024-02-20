import { getTimezoneOffset } from "date-fns-tz";
import { differenceInWeeks, setDay, startOfWeek } from 'date-fns';
import { YEAR } from './config';

const USER_TZ = Intl.DateTimeFormat().resolvedOptions().timeZone;
const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
const MONDAY = 1;

/**
 * Take a Date created in local time and converts it to Sydney time.
 *
 * The conversion logic may look a little backwards, but consider that a time in
 * UTC+10 actually occurs 10 hours before the same time in UTC. So, to convert
 * from 6PM UTC to 6PM AEST we need to subtract 10 hours.
 *
 * Example:
 *  We've created a Date at 6PM in the local TZ, say UTC-4 - this is 10PM UTC.
 *  This function will convert it to 6PM Sydney time i.e. 8AM UTC.
 *  We do this by adding -4 (local offset) and subtracting 10 (syd offset) hours
 */
export const toSydneyTime = (date: Date): Date => {
  const sydOffset = getTimezoneOffset('Australia/Sydney', date);
  const userOffset = getTimezoneOffset(USER_TZ, date);

  // Add user offset to force UTC then subtract syd offset to force Sydney time
  return new Date(date.getTime() + userOffset - sydOffset);
}

export const firstMonday = (year: number) => {
  const firstDay = new Date(year, 0, 1);
  return new Date(year, 0, 1 + ((8 - firstDay.getDay()) % 7));
}

/**
 * Create a date given a week number (0..52), day (full name) and time (HH:MM)
 */
export const createDate = (week: number, day: string, time: string) => {
  const dayNum = DAYS.indexOf(day);
  const [hours, minutes] = time.split(':').map(x => parseInt(x, 10));

  // Add weeks and day - if the day is greater than the number of days in a
  // month then JS just overflows it to the next
  return new Date(YEAR, 0, firstMonday(YEAR).getDate() + week * 7 + dayNum, hours, minutes);
}

/**
 * Calculate the week number of a given date (between 1 and 53)
 * Logic is kind of weird, but adapted from scwWeekNumber of Simple Calendar
 * Widget used by NSS so that our week numbers match up
 */
export const scwWeekNumber = (date: Date): number => {
  const year = date.getFullYear();

  const mondayOfDateWeek = startOfWeek(date, { weekStartsOn: MONDAY });

  // Monday of the first week (may actually be in the previous year)
  // This assumes the week starts on Sunday, so Sunday goes forward
  // but all else go backward
  const mondayOfFirstWeek = setDay(new Date(year, 0, 1), MONDAY);

  return differenceInWeeks(mondayOfDateWeek, mondayOfFirstWeek) + 1;
}

export const numWeeksInYear = (year: number) => {
  return scwWeekNumber(new Date(year, 11, 31)) - scwWeekNumber(firstMonday(year)) + 1;
}
