export type BuildingRow = { id: string; name: string };

export type RoomRow = {
    id: string;
    name: string;
    abbr?: string | null;
    usage: string; // "study"
    capacity?: number | null;
    school?: string | null;
    buildingId: string;
    floor?: string | null;
    seating?: string | null;
};