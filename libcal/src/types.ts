export type RoomBooking = {
  bookingType: string;
  name: string;
  roomId: string;
  start: Date;
  end: Date;
};

export type Room = {
  abbr: string;
  name: string;
  id: string;
  usage: string;
  capacity: number;
  school: string;
  buildingId: string;
  microphone: string[];
  accessibility: string[];
  audiovisual: string[];
  infotechnology: string[];
  writingMedia: string[];
  service: string[];
};

export type Library = {
  name: string;
  libcalCode: string;
  buildingId: string;
};
