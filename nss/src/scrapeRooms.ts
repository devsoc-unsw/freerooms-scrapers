import { Room, RoomMarkers } from "./types";
import { load } from "cheerio";
import nssFetch from "./nssFetch";
import { excludeRoom } from './exclusions';
import fs from "fs";

const ROOM_REGEX = /^K-[A-Z][0-9]{1,2}-.+$/;
const ROOM_OVERRIDES_PATH = "./roomOverrides.json";


// Returns an array of objects containing classroom info
// {
//   abbr: 'AnatLab7',
//   name: 'BiologicalSci TeachLab 7 Lvl1',
//   id: 'K-D26-106',
//   type: 'LAB',
//   capacity: 100,
//   school: 'BIOS'
//   lat: -33
//   long: 151
// }
const scrapeRooms = async (): Promise<Room[]> => {
  const response = await nssFetch('find_rooms');
  const $ = load(response.data);

  const classroomData: Room[] = [];
  $("form").find("tr.rowLowlight, tr.rowHighlight").each((_, e) => {
    const data = $(e).find("td");

    const room = {
      abbr: $(data[0]).text(),
      name: $(data[1]).text(),
      id: $(data[2]).text(),
      usage: $(data[3]).text(),
      capacity: parseInt($(data[4]).text()),
      school: $(data[5]).text(),
      buildingId: $(data[2]).text().split('-').slice(0, 2).join('-'),
      lat: 0,
      long: 0
    };

    if (room.id.match(ROOM_REGEX) && !excludeRoom(room)) {
      classroomData.push(room);
    }
  })

  overrideLocations(classroomData);
  return classroomData;
}

export default scrapeRooms;


const overrideLocations = (data: Room[]) => {
  const rawLocations = fs.readFileSync(ROOM_OVERRIDES_PATH, 'utf8');
  const locations = JSON.parse(rawLocations) as RoomMarkers[];

  // For each room in location data, replace the location in original data
  for (const room of locations) {
    const roomData = data.find(r => r.id === room.id);
    if (roomData) {
      roomData.lat = room.lat;
      roomData.long = room.long;
    }
  }
}