import pLimit from "p-limit";
import { parseList, parseDetail } from "./parse.js"
import type { BuildingRow, RoomRow } from "./types.js";
import { ALLOW_FAKE_COORDS } from "./config.js";

export async function buildRows(): Promise<{ buildings: BuildingRow[]; rooms: RoomRow[] }> {
    const list = await parseList();
    const limit = pLimit(6);
    const details = await Promise.all(list.map(i => limit(() => parseDetail(i.url))));

    const buildings = new Map<string, BuildingRow>();
    const rooms: RoomRow[] = [];

    for (let i = 0; i < list.length; i++) {
        const item = list[i];
        const det = details[i];

        const slug = new URL(item.url).pathname.split("/").filter(Boolean).pop()!;
        const roomName = det.title || item.name || slug.replace(/-/g, " ");

        const buildingId = det.buildingId;
        if (!buildingId) continue;


        if (!buildings.has(buildingId)) {
            const buildingName = det.title.split(" ")[0];

            const lat = det.coords?.lat ?? (ALLOW_FAKE_COORDS ? 0 : undefined);
            const long = det.coords?.long ?? (ALLOW_FAKE_COORDS ? 0 : undefined);

            if (lat !== undefined && long !== undefined) {
                buildings.set(buildingId, { id: buildingId, name: buildingName, lat, long, aliases: [] });
            }
        }


        rooms.push({
            id: `study-${slug}`,
            name: roomName,
            usage: "study",
            capacity: item.capacity ?? null,
            buildingId
        });
    }

    return { buildings: [...buildings.values()], rooms };
}