CREATE TYPE FloorTypeEnum AS ENUM (
    'Flat', 
    'Tiered', 
    'Other'
);

CREATE TYPE SeatingTypeEnum AS ENUM (
    'Movable', 
    'Fixed'
);

CREATE TABLE Rooms (
    "id"                 TEXT PRIMARY KEY,
    "name"               TEXT NOT NULL,
    "abbr"               TEXT NOT NULL,
    "usage"              TEXT NOT NULL,
    "capacity"           INTEGER NOT NULL,
    "school"             TEXT NOT NULL,
    "buildingId"         TEXT NOT NULL,
    "floor"              FloorTypeEnum,
    "seating"            SeatingTypeEnum,
    "microphone"         TEXT[] NOT NULL,
    "accessibility"      TEXT[] NOT NULL,
    "audiovisual"        TEXT[] NOT NULL,
    "infotechnology"     TEXT[] NOT NULL,
    "writingMedia"       TEXT[] NOT NULL,
    "service"            TEXT[] NOT NULL,
    FOREIGN KEY ("buildingId") REFERENCES Buildings("id") ON DELETE CASCADE
);