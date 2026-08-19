DELETE FROM Bookings
WHERE "start" >= '{0}'
  AND "start" < '{1}';