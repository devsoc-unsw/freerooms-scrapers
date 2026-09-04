CREATE TABLE BookingModules (
    "roomId"        TEXT NOT NULL,
    "occurrenceId"  TEXT NOT NULL,
    "moduleIndex"   INTEGER NOT NULL,

    "code"           TEXT NOT NULL,
    "name"           TEXT NOT NULL,
    "term"           TEXT,
    "career"         TEXT,

    PRIMARY KEY (
        "roomId",
        "occurrenceId",
        "moduleIndex"
    ),

    FOREIGN KEY (
        "roomId",
        "occurrenceId"
    )
    REFERENCES Bookings(
        "roomId",
        "occurrenceId"
    )
    ON DELETE CASCADE
);

CREATE INDEX booking_modules_code
    ON BookingModules ("code");

CREATE INDEX booking_modules_name
    ON BookingModules ("name");

CREATE INDEX booking_modules_occurrence
    ON BookingModules ("occurrenceId");