// Fetch an NSS page, adding all required request parameters
// Take in actual variable parameters like roomId
import axios from "axios";

const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
const NO_WEEKS = 52;

const nssFetch = async (
  page: "find_rooms" | "view_rooms" | "view_multirooms",
  params: Record<string, any> = {},
) => {
  // Create the URL for the given page
  const year = new Date().getFullYear();
  const url = `https://nss.cse.unsw.edu.au/tt/${page}.php?dbafile=${year}-KENS-COFA.DBA&campus=KENS`;

  // Add extra required params for each page
  let requiredParams: Record<string, any>;
  switch (page) {
    case "find_rooms":
      requiredParams = { days: DAYS, fr_time: 0, to_time: 0, search_rooms: "Search" };
      break;
    case "view_rooms":
      requiredParams = { view_room: "View" };
      break;
    case "view_multirooms":
      requiredParams = {}; // idk what this needs, we aren't using it rn tho
      break;
  }

  // If the year starts on Sunday then the first week of the year is 1, otherwise 2
  const yearStartsSunday = (new Date(year, 0, 1)).getDay() === 0;
  return axios.post(url, {
    ...params,
    ...requiredParams,
    fr_week: yearStartsSunday ? 1 : 2,
    to_week: yearStartsSunday ? NO_WEEKS : NO_WEEKS + 1,
  }, {
    headers: {
      "content-type": "application/x-www-form-urlencoded",
    },
  });
};

export default nssFetch;
