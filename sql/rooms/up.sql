CREATE TABLE Rooms (
    "id"            TEXT PRIMARY KEY,
    "name"          TEXT NOT NULL,
    "abbr"          TEXT NOT NULL,
    "usage"         TEXT NOT NULL,
    "capacity"      INTEGER NOT NULL,
    "school"        TEXT NOT NULL,
    "buildingId"    TEXT NOT NULL,
    FOREIGN KEY ("buildingId") REFERENCES Buildings("id") ON DELETE CASCADE
);
