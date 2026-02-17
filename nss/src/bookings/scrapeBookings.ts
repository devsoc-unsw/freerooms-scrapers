import fs from "fs";
import lodash from "lodash";
import { YEAR } from "../config";
import {
  CategoriesFilterResponse,
  Category,
  CategoryType,
  EventData,
  EventsFilterPayload,
  RemoteBooking,
  RoomBooking,
  ViewOptions,
  ViewOptionsResponse,
} from "../types";
import { parseRemoteBooking } from "./parseRemoteBooking";
import publishAPIClient from "./publishAPIClient";

// Constants
const UNSW_INSTITUTION_ID = "98c1cede-2447-4c14-92a3-7816107cd42b";
const CATEGORY_TYPE_IDS = {
  Location: "1e042cb1-547d-41d4-ae93-a1f2c3d34538",
  Zone: "1f3782b4-a328-44cd-9062-e7313650ba55",
  Department: "d334dcdb-6362-408b-b3e2-4dcd061d5654",
};


/***
 * Mapping raw response data to the relevent booking data
 */
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

/***
 * Given a category type (Location, Zone, Department) fetches all categories from publish.
 */
const getAllCategories = async (
  categoryType: CategoryType,
): Promise<Record<string, Category>> => {
  const url = `/CategoryTypes/${CATEGORY_TYPE_IDS[categoryType]}/Categories/FilterWithCache/${UNSW_INSTITUTION_ID}`;

  const results: Category[] = [];
  let totalPages = 1;
  let currentPage = 1;

  while (true) {
    const response = await publishAPIClient.rateLimitedPost(
      `${url}?pageNumber=${currentPage}`,
    );
    const data: CategoriesFilterResponse = response.data;

    totalPages = data.TotalPages;
    results.push(...data.Results);

    if (currentPage === totalPages) {
      break;
    }
    currentPage++;
  }

  return results.reduce<Record<string, Category>>((acc, category) => {
    acc[category.Identity] = category;
    return acc;
  }, {});
};

/***
 * Gets the view options for a specific year. 
 */
const getViewOptions = async (year: number): Promise<ViewOptions> => {
  const response = await publishAPIClient.rateLimitedGet(
    `/ViewOptions/${UNSW_INSTITUTION_ID}`,
  );
  const data: ViewOptionsResponse = response.data;

  return {
    DatePeriods: data.DatePeriods.filter((dp) => dp.Description === year),
    Days: data.Days,
    TimePeriods: data.TimePeriods.filter((tp) => tp.Description === "All Day"),
    Weeks: data.Weeks.filter((wk) =>
      wk.FirstDayInWeek.startsWith(String(year)),
    ),
  };
};

/***
 * Helper to generate all payloads for every request. 
 */
const generatePayloads = (
  categoryType: CategoryType,
  categoryIds: string[],
  viewOptions: ViewOptions,
): EventsFilterPayload[] => {
  const batchSize = 20;

  return lodash.chunk(categoryIds, batchSize).map((batch) => ({
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
  }));
};

/***
 * Fetches all raw events from publish. 
 */
const getEvents = async (
  categoryType: CategoryType,
  categoryIds: string[],
  viewOptions: ViewOptions,
): Promise<Record<string, EventData[]>> => {
  const requests = generatePayloads(categoryType, categoryIds, viewOptions).map(
    (payload) => {
      fs.writeFileSync("payload.json", JSON.stringify(payload, null, 2));
      return publishAPIClient.rateLimitedPost(
        `/CategoryTypes/Categories/Events/Filter/${UNSW_INSTITUTION_ID}`,
        payload,
      );
    },
  );

  const responses = await Promise.all(requests);

  const eventsByCategoryId: Record<string, EventData[]> = {};
  responses
    .map((response) => response.data)
    .forEach((data) => {
      for (const categoryEvents of data.CategoryEvents) {
        eventsByCategoryId[categoryEvents.Identity] = categoryEvents.Results;
      }
    });

  return eventsByCategoryId;
};

/***
 * Fetches all bookings from publish and maps it to the RoomBooking type.
 */
const scrapeBookings = async (): Promise<RoomBooking[]> => {
  const categories = await getAllCategories("Location");
  const kensingtonRoomIds = Object.keys(categories).filter((id) =>
    categories[id].Name.startsWith("K-"),
  );
  const viewOptions = await getViewOptions(YEAR);
  const events = await getEvents("Location", kensingtonRoomIds, viewOptions);
  return Object.values(events)
    .flat()
    .map(mapToRemoteBooking)
    .map((remoteBooking) => parseRemoteBooking(remoteBooking))
    .flat();
};

export default scrapeBookings;
