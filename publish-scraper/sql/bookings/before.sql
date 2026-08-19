DELETE FROM Bookings
WHERE "start" >= (
    '{0}-01-01 00:00:00'::timestamp
    AT TIME ZONE 'Australia/Sydney'
)
AND "start" < (
    '{1}-01-01 00:00:00'::timestamp
    AT TIME ZONE 'Australia/Sydney'
);