import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import { collectSessionIdentities } from "./sessionCollector";
import { SESSION_IDENTITIES_PATH } from "./constants";

const NSS_URL =
  "https://publish.unsw.edu.au/timetables?date=2025-12-22&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&searchText=K-";

const collectAllSessions = async () => {
  console.log("Collecting session identities (browser will open)…");

  const rooms = await scrapeRooms();
  const roomIds = rooms.map((r) => r.id);

  const abbrToSession = await collectSessionIdentities(NSS_URL, {
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
}

export default collectAllSessions