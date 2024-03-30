import { load } from "cheerio";
import nssFetch from "./nssFetch";
import { Facilities, FACILITIES_LIST } from "./types";

export const scrapeRoomFacilities = async (
  id: string
): Promise<Facilities> => {
  const additionalParams: Record<string, any> = {};
  additionalParams.show = ["show_facilities"];
  additionalParams.room = id;
  const response = await nssFetch("view_rooms", additionalParams);
  const $ = load(response.data);

  const additionalInformationData = {} as Facilities;
  for (const field of FACILITIES_LIST) {
    const data = $(`td:contains("${field}")`).parent();
    additionalInformationData[field] = cleanString(data.find('td.data').text())
  }

  return additionalInformationData;
};

const cleanString = (input: string): string[] => {
  return input.split('|').map(e => e.trim()).filter(e => e.length)
}