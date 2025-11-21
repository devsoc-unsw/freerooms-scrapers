import * as fs from 'fs';
import axios from "axios";

export type Coords = { lat?: number; long?: number; };

export async function getCoords(buildingId: string): Promise<Coords> {
    const writeObj = {};

    const requestString = `https://search.mazemap.com/search/equery/?q=${buildingId}&rows=1&page=0&start=0&boostbydistance=true&lat=-33.91608362810843&lng=151.23215845257903&z=1&withpois=true&withbuilding=true&withtype=true&withcampus=true&withfacets=true&campusid=111`;

    const response = await axios.get(requestString);
    const coords = response.data.result[0]?.point?.coordinates;
    const res: Coords = { lat: coords[0] ?? 0, long: coords[1] ?? 0 };
    if (!coords) {
        console.log(`No coordinates for room ${buildingId}`);
    }

    return res;
}
