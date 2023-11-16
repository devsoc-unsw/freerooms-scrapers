CREATE TYPE BookingTypeEnum AS ENUM ('LIB', 'CLASS', 'BLOCK', 'SOCIETY', 'MISC', 'INTERNAL');

CREATE TABLE Bookings (
    "id"            SERIAL PRIMARY KEY,
    "bookingType"   BookingTypeEnum,
    "name"          TEXT NOT NULL,
    "roomId"        TEXT NOT NULL,
    "start"         TIMESTAMPTZ NOT NULL,
    "end"           TIMESTAMPTZ NOT NULL,
    FOREIGN KEY ("roomId") REFERENCES Rooms("id") ON DELETE CASCADE
);

CREATE INDEX bookings_start_end ON Bookings ("start", "end");
