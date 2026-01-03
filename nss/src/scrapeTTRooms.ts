
import fs from "fs";
import fetchXlsx from "./fetchXlsx";
// import decodeXlsx from "./decodeXlsx";
// import { BookingsExcelRow, RawRoomBooking, UngroupedRoomBooking } from "./types";


// const OLD_TT_URL = "https://publish.sit1.unsw.edu.au/timetables?date=2026-02-16&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__";
const TT_URL = "https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538";

https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&timetableTypeFilter=160a6dc6-b4bc-0d75-0997-4fde27672921&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__a15e3653-1393-967d-57a9-fe5f75f0e1ac

// const TT_URL = "https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&timetableTypeFilter=160a6dc6-b4bc-0d75-0997-4fde27672921&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__";
const BATCH_SIZE = 20;

const scrapeTTRooms = async () => {
    const sessionIdentitiesRaw = fs.readFileSync("../sessionIdentities.json", "utf8");
    const sessionIdentities = JSON.parse(sessionIdentitiesRaw);

    // Process batches of 20 rooms
    for (let i = 0; i < 60; i += BATCH_SIZE) {
        const batch = sessionIdentities.slice(i, i + BATCH_SIZE);
        const url = TT_URL + batch.map((s: { Room: string; Identity: string }) => s.Identity).join("_");
        console.log(`Processing batch ${i} to ${i + BATCH_SIZE}...`);
        
        try {
            setTimeout(async () => {
                console.log(url);
                await fetchXlsx(url, `./${i}.xlsx`);
                // const ttBookings = decodeXlsx(`./${i}.xlsx`);
                // const bookings: RawRoomBooking[] = [];

                // for (const booking of ttBookings) {
                //     bookings.push({
                //         name: booking.name,
                //         day: booking.day,
                //         start: booking.start_time,
                //         weekPattern: 0,
                //         roomId: booking.allocated_location_name.split('-')[0].trimEnd(),
                //         end: booking.end_time,
                //     });
                // }

            }, 1000);
        } catch (error) {
            console.log(`Continuing with next batch...`);
        }
    }
}

scrapeTTRooms();