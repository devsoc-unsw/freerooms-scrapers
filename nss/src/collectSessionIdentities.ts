import fs from "fs";
import scrapeRooms from "./scrapeRooms";
import { collectSessionIdentities } from "./sessionCollector";

const NSS_URL =
  "https://publish.unsw.edu.au/timetables?date=2025-12-22&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&searchText=K-";

async function main() {
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
    "../sessionIdentities.json",
    JSON.stringify(updates, null, 4),
    "utf8"
  );
  console.log(
    `Saved ${updates.length} session identities to sessionIdentities.json`
  );
  //     }

  //     console.log(`Matched ${updates.length} identities to existing Rooms rows`);

  //     const requestConfig = {
  //         headers: {
  //             "Content-Type": "application/json",
  //             "X-Api-Key": HASURAGRES_API_KEY,
  //         },
  //     };

  //     console.time("Updating session identities");

  //     if (updates.length === 0) {
  //         console.warn("No matching rooms to update — skipping DB write");
  //     } else {
  //         await axios.post(
  //             `${HASURAGRES_URL}/batch_insert`,
  //             [
  //                 {
  //                     metadata: {
  //                         table_name: "Rooms",
  //                         columns: ["id", "sessionIdentity"],
  //                         write_mode: "update",
  //                         dryrun: DRYRUN,
  //                     },
  //                     payload: updates,
  //                 },
  //             ],
  //             requestConfig
  //         );
  //     }

  //     console.timeEnd("Updating session identities");

  //     console.log("Done");
  // }
}

main().catch((err) => {
  console.error("Failed:", err);
  process.exit(1);
});
