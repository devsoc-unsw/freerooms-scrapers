import pLimit from "p-limit";
import { parseList, parseDetail } from "./parse.js"
import type { BuildingRow, RoomRow } from "./types.js";

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
        const buildingId = det.buildingId ?? "UNKOWN";
        const roomName = det.title || item.name || slug.replace(/-/g, " ");

        if (det.buildingId && !buildings.has(det.buildingId)) {
            const buildingName = det.title.split(" ")[0];
            buildings.set(det.buildingId, { id: buildingId, name: buildingName });
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