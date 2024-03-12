/**
 * Buildings and room types that the scraper should ignore
 */
import { Building, Room } from './types';

// These buildings don't exist or aren't in the main campus
const EXCLUDED_BUILDINGS = [
  "K-BS38",
  "K-L5",
  "K-D9",
  "K-D10",
];

// Students can't use these rooms
const EXCLUDED_USAGES = [
  "EXAM",
  "OTHR"
]

// Students can't use these rooms
const EXCLUDED_SCHOOLS = [
  "ADMIN"
]

// These rooms don't exist (or exist under another name)
const EXCLUDED_ROOMS = [
  "K-J17-302A",
  "K-J17-302B",
  "K-J17-305A",
  "K-J17-305B",
  "K-G17-G3",
  "K-H6-G17",
  "K-E4-109",
  "K-E6-G",
  "K-E4-109A",
  "K-K17-B08"
]

export const excludeBuilding = (building: Building): boolean => {
  return EXCLUDED_BUILDINGS.includes(building.id);
}

export const excludeRoom = (room: Room): boolean => {
  return EXCLUDED_BUILDINGS.some(bId => room.id.startsWith(bId)) ||
         EXCLUDED_ROOMS.includes(room.id) ||
         EXCLUDED_USAGES.includes(room.usage) ||
         EXCLUDED_SCHOOLS.includes(room.school)
}
