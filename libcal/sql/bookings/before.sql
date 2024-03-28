-- Remove all bookings starting in or after the current timeslot
DELETE FROM Bookings
WHERE "bookingType" = 'LIB' AND "start" >= date_bin('30 minutes', now(), TIMESTAMPTZ '2001-01-01');

-- Truncate all bookings that go through the current timeslot
UPDATE Bookings
SET "end" = date_bin('30 minutes', now(), TIMESTAMPTZ '2001-01-01')
WHERE "bookingType" = 'LIB' AND "end" > date_bin('30 minutes', now(), TIMESTAMPTZ '2001-01-01');
