import { load } from "cheerio";
import nssFetch from "./nssFetch";
import {
  ScrapedFacilities,
  FACILITIES_LIST,
  MappedFacilities,
  FacilityFloor,
  FacilitySeating,
} from "./types";

export const scrapeRoomFacilities = async (
  id: string
): Promise<MappedFacilities> => {
  const additionalParams: Record<string, string> = {
    show: "show_facilities",
    room: id,
  };
  const response = await nssFetch("view_rooms", additionalParams);
  const $ = load(response.data);

  const additionalInformationData = {} as ScrapedFacilities;
  for (const field of FACILITIES_LIST) {
    const data = $(`td:contains("${field}")`).parent();
    additionalInformationData[field] = cleanString(data.find("td.data").text());
  }

  return facilitiesMapper(additionalInformationData);
};

const cleanString = (input: string): string[] => {
  return input
    .split("|")
    .map((e) => e.trim())
    .filter((e) => e.length);
};

const facilitiesMapper = (facilities: ScrapedFacilities): MappedFacilities => {
  const floorSeating = extractFloorSeating(facilities["Floor/seating"][0]);
  return {
    floor: floorSeating.floor,
    seating: floorSeating.seating,
    microphone: facilities.Microphone,
    accessibility: facilities.Accessibility,
    audiovisual: facilities["Audio-visual"],
    infotechnology: facilities["Info technology"],
    writingMedia: facilities["Writing media"],
    service: facilities.Services,
  };
};

const extractFloorSeating = (
  scrapedFloorSeating: string | undefined
): { floor: FacilityFloor | null; seating: FacilitySeating | null } => {
  switch (scrapedFloorSeating) {
    // yea, this case is moveable while everything else is movable..
    case "Flat floor node chairs moveable seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.MOVABLE,
      };
    case "Flat floor, fixed seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.FIXED,
      };
    case "Flat floor, movable seating":
      return {
        floor: FacilityFloor.FLAT,
        seating: FacilitySeating.MOVABLE,
      };
    case "Other Floor, Movable Seating":
      return {
        floor: FacilityFloor.OTHER,
        seating: FacilitySeating.MOVABLE,
      };
    case "Tiered Floor, Movable Seating":
      return {
        floor: FacilityFloor.TIERED,
        seating: FacilitySeating.MOVABLE,
      };
    case "Tiered floor, fixed seating":
      return {
        floor: FacilityFloor.TIERED,
        seating: FacilitySeating.FIXED,
      };
    default:
      console.warn(
        "Got unknown option for floor/seating combination!" +
          scrapedFloorSeating
      );
      return {
        floor: null,
        seating: null,
      };
  }
};
