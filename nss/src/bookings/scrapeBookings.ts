import axios from "axios";
import { EventData, RemoteBooking, RoomBooking, ViewOptions } from "../types";
import { parseRemoteBooking } from "./parseRemoteBooking";

const mapToRemoteBooking = (data: EventData): RemoteBooking => {
  return {
    moduleCode:
      data.ExtraProperties.find((property) => property.Name === "Module Name")
        ?.Value || "",
    moduleDescription:
      data.ExtraProperties.find(
        (property) => property.Name === "Module Description",
      )?.Value || "",
    name: data.Name,
    startTime: data.StartDateTime,
    endTime: data.EndDateTime,
    dates: data.WeekRanges,
    bookingType: data.EventType,
    allocatedLocationName:
      data.ExtraProperties.find((property) => property.Name === "Location Name")
        ?.Value || "",
  };
};

const API_BASE_URL =
  "https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public";
const UNSW_INSTITUTION_ID = "98c1cede-2447-4c14-92a3-7816107cd42b";
type CategoryType = "Location" | "Zone" | "Department";
const CATEGORY_TYPE_IDS = {
  Location: "1e042cb1-547d-41d4-ae93-a1f2c3d34538",
  Zone: "1f3782b4-a328-44cd-9062-e7313650ba55",
  Department: "d334dcdb-6362-408b-b3e2-4dcd061d5654",
};

const getAllCategories = async (
  categoryType: CategoryType,
): Promise<Record<string, any>> => {
  const url = `${API_BASE_URL}/CategoryTypes/${CATEGORY_TYPE_IDS[categoryType]}/Categories/FilterWithCache/${UNSW_INSTITUTION_ID}`;

  const results: any[] = [];
  let totalPages = 1;
  let currentPage = 1;

  while (true) {
    const response = await axios.post(`${url}?pageNumber=${currentPage}`);
    const data = response.data;

    totalPages = data.TotalPages;
    results.push(...data.Results);

    if (currentPage === totalPages) {
      break;
    }
    currentPage++;
  }

  return results.reduce((acc, category) => {
    acc[category.Identity] = category;
    return acc;
  }, {});
};

const getViewOptions = async (
  year: number,
): Promise<ViewOptions> => {
  const response = await axios.get(
    `${API_BASE_URL}/ViewOptions/${UNSW_INSTITUTION_ID}`,
  );
  const data = response.data;

  return {
    DatePeriods: data.DatePeriods.filter(
      (dp: any) => dp.Description === year,
    ),
    Days: data.Days,
    TimePeriods: data.TimePeriods.filter(
      (tp: any) => tp.Description === "All Day",
    ),
    Weeks: data.Weeks.filter((wk: any) =>
      wk.FirstDayInWeek.startsWith(year),
    ),
  };
};

const getEvents = async (
  categoryType: CategoryType,
  categoryIds: string[],
  viewOptions: ViewOptions,
) => {
  const eventsByCategoryId: Record<string, EventData[]> = {};
  const batchSize = 20;

  for (let i = 0; i < categoryIds.length; i += batchSize) {
    const batch = categoryIds.slice(i, i + batchSize);

    const payload = {
      CategoryTypesWithIdentities: [
        {
          CategoryTypeIdentity: CATEGORY_TYPE_IDS[categoryType],
          CategoryIdentities: batch,
        },
      ],
      ViewOptions: viewOptions,
      FetchBookings: false,
      FetchPersonalEvents: false,
      PersonalIdentities: [],
    };

    const response = await axios.post(
      `${API_BASE_URL}/CategoryTypes/Categories/Events/Filter/${UNSW_INSTITUTION_ID}`,
      payload,
    );

    for (const categoryEvents of response.data.CategoryEvents) {
      eventsByCategoryId[categoryEvents.Identity] = categoryEvents.Results;
    }
  }

  return eventsByCategoryId;
};

const scrapeBookings = async (): Promise<RoomBooking[]> => {
  const categories = await getAllCategories("Location");
  const kensingtonRoomIds = Object.keys(categories).filter((id) =>
    categories[id].Name.startsWith("K-"),
  );
  const viewOptions = await getViewOptions(2026);
  const events = await getEvents("Location", kensingtonRoomIds, viewOptions);
  return Object.values(events)
    .flat()
    .map(mapToRemoteBooking)
    .map((remoteBooking) => parseRemoteBooking(remoteBooking))
    .flat();
};

export default scrapeBookings;