CREATE TABLE Buildings (
    "id"        TEXT PRIMARY KEY,
    "name"      TEXT NOT NULL,
    "lat"       DOUBLE PRECISION NOT NULL,
    "long"      DOUBLE PRECISION NOT NULL,
    "aliases"   TEXT[] NOT NULL
);
