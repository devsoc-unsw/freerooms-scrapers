-- The scraper combines adjacent bookings, but this handles the case where a
-- booking is truncated by the SQL before, and then has a continuation inserted

-- Find all bookings that can be combined
CREATE TEMPORARY TABLE CombinedBookings AS (
    SELECT b1."bookingType" AS booking_type,
           b1.name AS name,
           b1."roomId" AS room_id,
           b1.start AS start_1,
           b1.end AS end_1,
           b2.start AS start_2,
           b2.end AS end_2
    FROM   Bookings b1
           JOIN Bookings b2 ON b1.end = b2.start AND b1."roomId" = b2."roomId"
    WHERE  b1."bookingType" = 'LIB' AND b2."bookingType" = 'LIB'
);

-- Remove the original bookings
DELETE FROM Bookings
WHERE ("roomId", "start", "end") IN (
    SELECT room_id, start_1, end_1 FROM CombinedBookings
    UNION
    SELECT room_id, start_2, end_2 FROM CombinedBookings
);

-- Insert the new combined bookings
INSERT INTO Bookings ("bookingType", "name", "roomId", "start", "end")
SELECT booking_type, name, room_id, start_1, end_2
FROM CombinedBookings;

DROP TABLE CombinedBookings;
