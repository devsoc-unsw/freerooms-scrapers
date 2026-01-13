export type UngroupedRoomBooking = {
  name: string;
  day: string;
  start: string;
  weekPattern: number;
};

export type RawRoomBooking = UngroupedRoomBooking & {
  roomId: string;
  end: string;
};

export type RoomBooking = {
  bookingType: string;
  name: string;
  roomId: string;
  start: Date;
  end: Date;
};

export const FACILITIES_LIST = [
  "Floor/seating",
  "Microphone",
  "Accessibility",
  "Audio-visual",
  "Info technology",
  "Writing media",
  "Services",
] as const;

export type ScrapedFacilities = Record<
  (typeof FACILITIES_LIST)[number],
  string[]
>;

// remember to change the sql enum type as well!
export enum FacilityFloor {
  FLAT = "Flat",
  TIERED = "Tiered",
  OTHER = "Other",
}

export enum FacilitySeating {
  MOVABLE = "Movable",
  FIXED = "Fixed",
}

export type MappedFacilities = {
  floor: FacilityFloor | null;
  seating: FacilitySeating | null;
  microphone: string[];
  accessibility: string[];
  audiovisual: string[];
  infotechnology: string[];
  writingMedia: string[];
  service: string[];
};

export type Room = {
  abbr: string;
  name: string;
  id: string;
  usage: string;
  capacity: number;
  school: string;
  buildingId: string;
  lat: number;
  long: number;
};

export type ParsedName = {
  bookingType: string;
  name: string;
};

export type NameParser = {
  pattern: RegExp;
  parser: (matchGroups: Record<string, string>) => ParsedName;
};

export type Building = {
  name: string;
  id: string;
  lat: number;
  long: number;
  aliases: string[];
};

export type OverrideData = {
  buildings: Building[];
};

export type RoomMarkers = {
  id: string;
  lat: number;
  long: number;
};

export type BookingsExcelRow = {
  module_code: string;
  module_description: string;
  name: string;
  class_size: number;
  day:
    | "Monday"
    | "Tuesday"
    | "Wednesday"
    | "Thursday"
    | "Friday"
    | "Saturday"
    | "Sunday";
  duration: string;
  start_time: string;
  end_time: string;
  dates: string;
  allocated_location_name: string;
  weeks: string;
};

export type RoomSessionIdentity = {
  id: string;
  sessionIdentity: string;
};
