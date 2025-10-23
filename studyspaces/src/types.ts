export type BuildingRow = { id: string; name: string; lat: number; long: number; aliases: string[] };

export type RoomRow = {
    id: string;
    name: string;
    abbr: string;
    usage: string; // "study"
    capacity: number;
    school: string;
    buildingId: string;
    floor?: string | null;
    seating?: string | null;
    microphone: string[];
    accessibility: string[];
    audiovisual: string[];
    infotechnology: string[];
    writingMedia: string[];
    service: string[];
    lat: number;
    long: number;
};