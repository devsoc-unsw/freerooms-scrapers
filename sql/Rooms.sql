CREATE TABLE  IF NOT EXISTS Rooms (
	"id"			TEXT PRIMARY KEY,
	"name"			TEXT NOT NULL,
	"abbr"			TEXT NOT NULL,	
	"usage"			TEXT NOT NULL,
	"capacity"		INTEGER NOT NULL,
	"school"		TEXT NOT NULL,
	"buildingid"	TEXT NOT NULL,
	FOREIGN KEY ("buildingid") REFERENCES Buildings("id") ON DELETE CASCADE
);
