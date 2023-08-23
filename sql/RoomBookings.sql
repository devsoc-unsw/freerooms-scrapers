DO $$ BEGIN
    CREATE TYPE BookingTypeEnum AS ENUM ('CLASS', 'BLOCK', 'SOCIETY', 'MISC', 'INTERNAL');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE  IF NOT EXISTS RoomBookings (
    "id"            SERIAL PRIMARY KEY,
    "bookingType"   BookingTypeEnum,
    "name"          TEXT NOT NULL,
    "roomId"        TEXT NOT NULL,
    "start"         TIMESTAMP NOT NULL,
    "end"           TIMESTAMP NOT NULL,
	FOREIGN KEY ("roomId") REFERENCES Rooms("id") ON DELETE CASCADE
);
