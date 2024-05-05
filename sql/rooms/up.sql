CREATE TYPE FloorTypeEnum AS ENUM (
    'Flat', 
    'Tiered', 
    'Other'
);

CREATE TYPE SeatingTypeEnum AS ENUM (
    'Movable', 
    'Fixed'
);

-- CREATE TYPE MicrophoneTypeEnum AS ENUM (
--     'dual radio microphone', 
--     'lectern (fixed)', 
--     'neck (navalier)', 
--     'radio microphone'
-- );

-- CREATE TYPE AccessibilityTypeEnum AS ENUM (
--     'hearing loop',
--     'power at wall',
--     'power at seat',
--     'ventilation - air conditioning',
--     'weekend access',
--     'wheelchair access - student',
--     'wheelchair access - teaching',
--     'wheelchair tablet'
-- );

-- CREATE TYPE AudiovisualTypeEnum AS ENUM (
--     'dvd player',
--     'document camera',
--     'dual slide projectors',
--     'projector, 16mm',
--     'slide projector (one or more)',
--     'television monitor',
--     'video cassette recorder'
-- );

-- CREATE TYPE InfotechnologyTypeEnum AS ENUM (
--     'collaborative learning space (low tech)',
--     'digital annotation monitor',
--     'dual video data projectors',
--     'hybrid teaching space',
--     'it lectern',
--     'it laptop connection',
--     'it laptop connection (vga)',
--     'interactive learning space (high tech)',
--     'interactive display monitor',
--     'lectopia (ilecture) enable podcasting',
--     'lectopia (ilecture) make publically accessible',
--     'lecture capture venue',
--     'lecture capture with screen capture (limited availability)',
--     'lecture capture with video feed',
--     'room linking',
--     'video data projector',
--     'web camera with microphone'
-- );

-- CREATE TYPE WritingMediaTypeEnum AS ENUM (
--     'blackboard',
--     'blackboard or whiteboard',
--     'blackboard, wide',
--     'dual overhead projectors',
--     'whiteboard'
-- );

-- CREATE TYPE ServiceTypeEnum AS ENUM (
--     'break out rooms',
--     'cats casual space',
--     'demonstration benches',
--     'demonstration gas taps and sinks'  
-- );


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
