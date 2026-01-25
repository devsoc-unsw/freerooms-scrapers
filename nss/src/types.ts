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

export type RemoteBooking = {
  moduleCode: string;
  moduleDescription: string;
  name: string;
  bookingType: string;
  startTime: string;
  endTime: string;
  dates: string;
  allocatedLocationName: string;
};

export type ExtraProperty = {
  Name: string;
  Value: string;
};

export type EventData = {
  StartDateTime: string;
  EndDateTime: string;
  Location: string;
  Description: string;
  Name: string;
  EventType: string;
  ExtraProperties: ExtraProperty[];
  WeekRanges: string;
  WeekLabels: string;
};

export type ViewOptions = {
  DatePeriods: any[];
  Days: any[];
  TimePeriods: any[];
  Weeks: any[];
}