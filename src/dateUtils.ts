import { getTimezoneOffset } from "date-fns-tz";
import { startOfWeek } from 'date-fns';

const USER_TZ = Intl.DateTimeFormat().resolvedOptions().timeZone;
const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];

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
  const year = new Date().getFullYear();

  // Add weeks and day - if the day is greater than the number of days in a
  // month then JS just overflows it to the next
  return new Date(year, 0, firstMonday(year).getDate() + week * 7 + dayNum, hours, minutes);
}

/**
 * Adapted from function on the NSS site to calculate week number (in scw.js)
 */
const scwWeekNumberEpoch = true;
const scwWeekNumberBaseDay = 1;
export const scwWeekNumber = (scwInDate: Date): number => {
  // The base day in the week of the input date
  const scwBaseYear = scwInDate.getFullYear();
  const scwInDateWeekBase = startOfWeek(scwInDate, { weekStartsOn: scwWeekNumberBaseDay });

  // The first Base Day in the year
  // GRW which year is the origin?
  const scwFirstBaseDay = new Date(scwWeekNumberEpoch ? scwBaseYear : scwInDateWeekBase.getFullYear(), 0, 1);
  scwFirstBaseDay.setDate(scwFirstBaseDay.getDate()
    - scwFirstBaseDay.getDay()
    + scwWeekNumberBaseDay
  );

  if (!scwWeekNumberEpoch && scwFirstBaseDay < new Date(scwInDateWeekBase.getFullYear(), 0, 1)) {
    scwFirstBaseDay.setDate(scwFirstBaseDay.getDate() + 7);
  }

  // Start of Week 01
  const scwStartWeekOne = startOfWeek(scwFirstBaseDay, { weekStartsOn: scwWeekNumberBaseDay });

  // Subtract the date of the current week from the date of the
  // first week of the year to get the number of weeks in
  // milliseconds.  Divide by the number of milliseconds
  // in a week then round to no decimals in order to remove
  // the effect of daylight saving.  Add one to make the first
  // week, week 1.
  return Math.round((scwInDateWeekBase.getTime() - scwStartWeekOne.getTime()) / 604800000) + 1;
}

export const numWeeksInYear = (year: number) => {
  return scwWeekNumber(new Date(year, 11, 31)) - scwWeekNumber(firstMonday(year)) + 1;
}
