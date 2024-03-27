-- Deletes all non-library bookings in the current year
-- {0} will be substituted with the start of the year
-- {1} will be substituted with the end of the year
DELETE FROM Bookings
WHERE "bookingType" <> 'LIB' AND "start" >= '{0}' AND "end" < '{1}';
