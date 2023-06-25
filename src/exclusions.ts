/**
 * Buildings and room types that the scraper should ignore
 */
import { Building, Room } from './types';

const EXCLUDED_BUILDINGS = [
  "K-BS38",
  "K-L5",
  "K-D9",
  "K-D10"
];

const EXCLUDED_USAGES = [
  "EXAM",
  "OTHR"
]

const EXCLUDED_SCHOOLS = [
  "ADMIN"
]

export const excludeBuilding = (building: Building): boolean => {
  return EXCLUDED_BUILDINGS.includes(building.id);
}

export const excludeRoom = (room: Room): boolean => {
  return EXCLUDED_BUILDINGS.some(bId => room.id.startsWith(bId)) ||
         EXCLUDED_USAGES.includes(room.usage) ||
         EXCLUDED_SCHOOLS.includes(room.school)
}
