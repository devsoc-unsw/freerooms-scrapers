export type RoomBooking = {
    bookingType: string;
    name: string;
    roomId: string;
    start: Date;
    end: Date;
  }

  export type Room = {
    abbr: string;
    name: string;
    id: string;
    usage: string;
    capacity: number;
    school: string;
    buildingId: string;
  }
