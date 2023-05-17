import { utcToZonedTime, zonedTimeToUtc } from "date-fns-tz";

const USER_TZ = Intl.DateTimeFormat().resolvedOptions().timeZone;
export const toSydneyTime = (date: Date): Date => {
  // Convert from local to UTC
  const utcDate = utcToZonedTime(date, USER_TZ);

  // Convert from UTC to Sydney
  return zonedTimeToUtc(utcDate, 'Australia/Sydney');
}
