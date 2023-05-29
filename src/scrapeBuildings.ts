import { Building } from "./types";
import { load } from "cheerio";
import nssFetch from "./nssFetch";
import fs from "fs";
import { LocationData } from "./types";

const BUILDING_REGEX = /^K-[A-Z][0-9]{1,2}$/;

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
    if (!id?.match(BUILDING_REGEX)) return;
    buildings.push({ name, id, lat: 0, long: 0 });
  });

  overrideLocations(buildings);
  return buildings;
}

// Manually override the building locations in the database
const overrideLocations = (data: Building[]) => {
  const rawLocations = fs.readFileSync('./buildingLocations.json', 'utf8');
  const locations = JSON.parse(rawLocations) as LocationData;

  // For each building in location data, replace the location in original data
  for (const building of locations.buildings) {
    const buildingData = data.find(b => b.id === building.id);
    if (buildingData) {
      buildingData.lat = building.lat;
      buildingData.long = building.long;
    }
  }
}

export default scrapeBuildings;
