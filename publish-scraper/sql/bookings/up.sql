CREATE TYPE BookingTypeEnum AS ENUM (
    'LECTURE',
    'TUTORIAL',
    'LABORATORY',
    'TUTORIAL_LABORATORY',
    'WORKSHOP',
    'SEMINAR',
    'STUDIO',
    'CLASS',
    'EXAMS',
    'SOCIETY',
    'INTERNAL',
    'BLOCK',
    'MISC',
    'OTHER'
);

CREATE TABLE Bookings (
    "roomId"        TEXT NOT NULL,
    "occurrenceId"  TEXT NOT NULL,
    "eventId"       TEXT NOT NULL,

    "bookingType"   BookingTypeEnum NOT NULL,
    "name"          TEXT NOT NULL,
    "rawName"       TEXT NOT NULL,
    "eventType"     TEXT NOT NULL,

    "start"         TIMESTAMPTZ NOT NULL,
    "end"           TIMESTAMPTZ NOT NULL,

    "plannedSize"   INTEGER,
    "source"        TEXT,
    "lastModified"  TIMESTAMPTZ,

    PRIMARY KEY ("roomId", "occurrenceId"),

    FOREIGN KEY ("roomId")
        REFERENCES Rooms("id")
        ON DELETE CASCADE,

    CHECK ("end" > "start")
);

CREATE INDEX bookings_start_end
    ON Bookings ("start", "end");

CREATE INDEX bookings_room_start_end
    ON Bookings ("roomId", "start", "end");

CREATE INDEX bookings_occurrence_id
    ON Bookings ("occurrenceId");

CREATE INDEX bookings_event_id
    ON Bookings ("eventId");

CREATE INDEX bookings_type_name
    ON Bookings ("bookingType", "name");