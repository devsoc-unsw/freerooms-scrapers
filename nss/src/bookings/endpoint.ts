import axios from "axios";
import collectAllSessions from "../collectSessionIdentities";
import { BookingsExcelRow } from "../types";

type RemoteCategoryEvent = {
  Identity: string;
  Name: string;
  Results: RemoteBooking[];
};

type RemoteAPIResponse = {
  CategoryEvents: RemoteCategoryEvent[];
};

type ExtraProperty = {
  Name: string;
  Value: string;
};

type RemoteBooking = {
  StartDateTime: string;
  EndDateTime: string;
  Location: string;
  Description: string;
  Name: string;
  EventType: string;
  ExtraProperties: ExtraProperty[];
  WeekRanges: string;
  WeekLabels: string;
};

type Payload = {
  ViewOptions: {
    Days: { DayOfWeek: number }[];
  };
  CategoryTypesWithIdentities: {
    CategoryTypeIdentity: string;
    CategoryIdentities: string[];
  }[];
};

const mapToRemoteBooking = (data: RemoteBooking): BookingsExcelRow => {
  return {
    module_code:
      data.ExtraProperties.find((property) => property.Name === "Module Name")
        ?.Value || "",
    module_description:
      data.ExtraProperties.find(
        (property) => property.Name === "Module Description",
      )?.Value || "",
    name: data.Name,
    start_time: data.StartDateTime,
    end_time: data.EndDateTime,
    dates: data.WeekRanges,
    booking_type: data.EventType,
    allocated_location_name:
      data.ExtraProperties.find((property) => property.Name === "Location Name")
        ?.Value || "",
  };
};

const payload = {
  ViewOptions: {
    Days: [
      { DayOfWeek: 0 },
      { DayOfWeek: 1 },
      { DayOfWeek: 2 },
      { DayOfWeek: 3 },
      { DayOfWeek: 4 },
      { DayOfWeek: 5 },
      { DayOfWeek: 6 },
    ],
  },
  CategoryTypesWithIdentities: [
    {
      CategoryTypeIdentity: "1e042cb1-547d-41d4-ae93-a1f2c3d34538",
      CategoryIdentities: ["78edff89-9837-22dd-a4bc-44972974f3ca"],
    },
  ],
};

const generatePayload = async (): Promise<Payload> => {
  const roomIdentities = (await collectAllSessions()).map(
    ({ sessionIdentity }) => sessionIdentity,
  );
  return Promise.resolve({
    ViewOptions: {
      Days: [
        { DayOfWeek: 0 },
        { DayOfWeek: 1 },
        { DayOfWeek: 2 },
        { DayOfWeek: 3 },
        { DayOfWeek: 4 },
        { DayOfWeek: 5 },
        { DayOfWeek: 6 },
      ],
    },
    CategoryTypesWithIdentities: [
      {
        CategoryTypeIdentity: "1e042cb1-547d-41d4-ae93-a1f2c3d34538",
        CategoryIdentities: roomIdentities,
      },
    ],
  });
};

const getBookings = (): Promise<BookingsExcelRow[]> => {
  return generatePayload()
    .then((payload) =>
      axios.post(
        "https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public/CategoryTypes/Categories/Events/Filter/98c1cede-2447-4c14-92a3-7816107cd42b?startRange=2025-12-31T13:00:00.000Z&endRange=2030-02-03T12:59:59.999Z",
        payload,
      ),
    )
    .then((resp) => resp.data)
    .then((data: RemoteAPIResponse) =>
      data.CategoryEvents.map((categoryEvent) =>
        categoryEvent.Results.map(mapToRemoteBooking),
      ),
    )
    .then((bookings) => bookings.flat());
};

(async () => {
  console.log(
    (await getBookings()).length,
  );
})();
