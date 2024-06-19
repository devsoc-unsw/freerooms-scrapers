// Fetch an NSS page, adding all required request parameters
// Take in actual variable parameters like roomId
import axios from "axios";
import { firstMonday, scwWeekNumber } from './dateUtils';
import { YEAR } from './config';

const DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];

const nssFetch = async (
  page: "find_rooms" | "view_rooms" | "view_multirooms",
  params: Record<string, any> = {},
) => {
  // Create the URL for the given page
  const url = `https://nss.cse.unsw.edu.au/tt/${page}.php?dbafile=${YEAR}-KENS-COFA.DBA&campus=KENS`;

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

  return axios.post(url, {
    ...params,
    ...requiredParams,
    fr_week: scwWeekNumber(firstMonday(YEAR)),
    to_week: scwWeekNumber(new Date(YEAR, 11, 31)),
  }, {
    headers: {
      "content-type": "application/x-www-form-urlencoded",
    },
  });
};

export default nssFetch;
