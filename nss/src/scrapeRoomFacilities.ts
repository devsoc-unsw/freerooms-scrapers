import { load } from "cheerio";
import nssFetch from "./nssFetch";
import { ScrapedFacilities, FACILITIES_LIST, MappedFacilities, FacilityFloor, FacilitySeating } from "./types";

export const scrapeRoomFacilities = async (
  id: string
): Promise<MappedFacilities> => {
  const additionalParams: Record<string, string> = {
    show: "show_facilities",
    room: id
  };
  const response = await nssFetch("view_rooms", additionalParams);
  const $ = load(response.data);

  const additionalInformationData = {} as ScrapedFacilities;
  for (const field of FACILITIES_LIST) {
    const data = $(`td:contains("${field}")`).parent();
    additionalInformationData[field] = cleanString(data.find('td.data').text())
  }

  return facilitiesMapper(additionalInformationData);
};

const cleanString = (input: string): string[] => {
  return input.split('|').map(e => e.trim()).filter(e => e.length)
}

const facilitiesMapper = (facilities: ScrapedFacilities): MappedFacilities => {
  const floorSeating = extractFloorSeating(facilities["Floor/seating"][0]);
  return {
    floor: floorSeating.floor,
    seating: floorSeating.seating,
    microphone: facilities.Microphone.map(e => e.toLowerCase()),
    accessibility: facilities.Accessibility.map(e => e.toLowerCase()),
    audiovisual: facilities["Audio-visual"].map(e => e.toLowerCase()),
    infotechnology: facilities["Info technology"].map(e => e.toLowerCase()),
    writingMedia: facilities["Writing media"].map(e => e.toLowerCase()),
    service: facilities.Services.map(e => e.toLowerCase())
  }
}

const extractFloorSeating = (scrapedFloorSeating: string | undefined): { floor: FacilityFloor | null, seating: FacilitySeating | null } => {
  if (!scrapedFloorSeating) {
    return {
      floor: null,
      seating: null
    }
  }

  switch (scrapedFloorSeating) {
    // yea, this case is moveable while everything else is movable..
    case "Flat floor node chairs moveable seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.MOVABLE
      }
    case "Flat floor, fixed seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.FIXED
      }
    case "Flat floor, movable seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.MOVABLE
      }
    case "Other Floor, Movable Seating":
      return {
        floor: FacilityFloor.OTHER,
        seating: FacilitySeating.MOVABLE
      }
    case "Tiered Floor, Movable Seating":
      return {
        floor: FacilityFloor.TIERED,
        seating: FacilitySeating.MOVABLE
      }
    case "Tiered floor, fixed seating":
      return {
        floor: FacilityFloor.TIERED,
        seating: FacilitySeating.FIXED
      }
    default:
      throw new Error("Not an expected scraped floor/seating type!")
  }
}