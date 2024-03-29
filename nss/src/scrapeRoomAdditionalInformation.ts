import { load } from "cheerio";
import nssFetch from "./nssFetch";

const fieldList = ["Accessibility", "Audio-visual", "Info technology", "Writing media", "Services"] as const;

type AdditionalInformation = Record<typeof fieldList[number], string[]>

const scrapeRoomAdditionalInformation = async (
  id: string
): Promise<AdditionalInformation> => {
  const additionalParams: Record<string, any> = {};
  additionalParams.show = ["show_facilities"];
  additionalParams.room = id;
  const response = await nssFetch("view_rooms", additionalParams);
  const $ = load(response.data);

  const additionalInformationData = {} as AdditionalInformation;
  for (const field of fieldList) {
    const data = $(`td:contains("${field}")`).parent();
    additionalInformationData[field] = cleanString(data.find('td.data').text())
  }

  return additionalInformationData;
};

const cleanString = (input: string): string[] => {
  return input.split('|').map(e => e.trim()).filter(e => e.length)
}