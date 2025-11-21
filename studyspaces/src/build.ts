import pLimit from "p-limit";
import { parseList, parseDetail } from "./parse.js"
import type { BuildingRow, RoomRow } from "./types.js";
import { ALLOW_FAKE_COORDS } from "./config.js";
import { getCoords } from "./coords.js";

export async function buildRows(): Promise<{ buildings: BuildingRow[]; rooms: RoomRow[] }> {
    const list = await parseList();
    const limit = pLimit(6);
    const details = await Promise.all(list.map(i => limit(() => parseDetail(i.url))));

    const buildings = new Map<string, BuildingRow>();
    const rooms: RoomRow[] = [];

    for (let i = 0; i < list.length; i++) {
        const item = list[i];
        const det = details[i];

        const buildingId = det.buildingId;
        if (!buildingId) continue;

        const slug = new URL(item.url).pathname.split("/").filter(Boolean).pop()!;
        const roomName = det.title || item.name || slug.replace(/-/g, " ");
        const capacity = Number.isFinite(item.capacity!) ? (item.capacity as number) : 0;


        if (!buildings.has(buildingId)) {
            const buildingName = det.title.split(" ")[0];

            const coords = await getCoords(buildingId);
            const lat = coords?.lat ?? (ALLOW_FAKE_COORDS ? 0 : undefined);
            const long = coords?.long ?? (ALLOW_FAKE_COORDS ? 0 : undefined);

            if (lat === undefined || long === undefined) continue;
            buildings.set(buildingId, { id: buildingId, name: buildingName, lat, long, aliases: [] });
        }

        const building = buildings.get(buildingId);
        const lat = building?.lat ?? (ALLOW_FAKE_COORDS ? 0 : undefined);
        const long = building?.long ?? (ALLOW_FAKE_COORDS ? 0 : undefined);
        if (lat === undefined || long === undefined) continue;


        rooms.push({
            id: `study-${slug}`,
            name: roomName,
            abbr: roomName,
            usage: "study",
            capacity: capacity,
            school: "General",
            buildingId,
            microphone: [],
            accessibility: [],
            audiovisual: [],
            infotechnology: [],
            writingMedia: [],
            service: [],
            lat,
            long
        });
    }

    return { buildings: [...buildings.values()], rooms };
}