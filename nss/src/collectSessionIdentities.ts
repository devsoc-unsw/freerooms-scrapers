import fs from "fs";
import { SESSION_IDENTITIES_PATH } from "./constants";
import scrapeRooms from "./scrapeRooms";
import { collectSessionIdentities } from "./sessionCollector";

const PUBLISH_URL =
  "https://publish.unsw.edu.au/timetables?date=2025-12-22&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&searchText=K-";


/**
 * Collects or loads the session identities, and stores them in a json file,
 * it also filters the rooms for what exists in the database
 */
const collectAllSessions = async () => {
  // TODO: In case new rooms are added maybe periodically expire this?
  if (fs.existsSync(SESSION_IDENTITIES_PATH)) {
    console.log(
      "Session identities already exists in disk so fetching from disk"
    );
    return JSON.parse(fs.readFileSync(SESSION_IDENTITIES_PATH, "utf8"));
  }
  console.log(
    "Session identities does not exist in disk so fetching from publish"
  );
  console.log("Collecting session identities (browser will open)…");

  const rooms = await scrapeRooms();
  const roomIds = rooms.map((r) => r.id);

  const abbrToSession = await collectSessionIdentities(PUBLISH_URL, {
    batchSize: 20,
    maxRooms: Infinity,
    clickDelayMs: 1,
  });

  console.log(`Collected ${abbrToSession.size} session identities`);

  const updates = [];
  for (const [id, sessionIdentity] of abbrToSession.entries()) {
    if (!roomIds.includes(id)) continue;
    if (!id) continue;
    updates.push({ id, sessionIdentity });
  }

  fs.writeFileSync(
    SESSION_IDENTITIES_PATH,
    JSON.stringify(updates, null, 4),
    "utf8"
  );
  console.log(
    `Saved ${updates.length} session identities to sessionIdentities.json`
  );

  return updates;
};

export default collectAllSessions;
