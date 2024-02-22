-- Delete all non-library rooms that no longer exist
-- {0} will be substituted with list of all scraped room ids
DELETE FROM Rooms
WHERE "usage" <> 'LIB' AND "id" NOT IN ({0});
