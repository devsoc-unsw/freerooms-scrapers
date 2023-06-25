import { Building } from "./types";
import { load } from "cheerio";
import nssFetch from "./nssFetch";
import fs from "fs";
import { OverrideData } from "./types";
import { excludeBuilding } from './exclusions';

const BUILDING_REGEX = /^K-[A-Z][0-9]{1,2}$/;
const BUILDING_OVERRIDES_PATH = "./buildingOverrides.json";

// Returns an array of objects containing building info
// {
//   name: 'Ainsworth Building',
//   id: 'K-J17'
//   lat: -33
//   long: 151
// }
const scrapeBuildings = async (): Promise<Building[]> => {
  const response = await nssFetch('view_multirooms');
  const $ = load(response.data);

  const buildings: Building[] = [];
  $('select[name="building"]').find('option').each((_, e) => {
    const [name, id] = $(e).text().split(' - ');
    const building = {
      name: cleanName(name),
      id,
      lat: 0,
      long: 0,
      aliases: []
    }
    if (building.id?.match(BUILDING_REGEX) && !excludeBuilding(building)) {
      buildings.push(building);
    }
  });

  overrideLocations(buildings);
  return buildings;
}

// Manually override the building locations in the database
const overrideLocations = (data: Building[]) => {
  const rawLocations = fs.readFileSync(BUILDING_OVERRIDES_PATH, 'utf8');
  const locations = JSON.parse(rawLocations) as OverrideData;

  // For each building in location data, replace the location in original data
  for (const building of locations.buildings) {
    const buildingData = data.find(b => b.id === building.id);
    if (buildingData) {
      buildingData.lat = building.lat;
      buildingData.long = building.long;
      buildingData.aliases.push(...building.aliases);
    }
  }
}

const cleanName = (name: string): string => {
  return name
    .replace(/^[A-Z][0-9]{1,2} /, "")  // Get rid of leading grid refs
    .replace(/\ufffd/g, " ")           // Replace the weird chars in red centre
    .replace(/Undercroft/, "")         // Get rid of "Undercroft"
    .trim();
}

export default scrapeBuildings;
