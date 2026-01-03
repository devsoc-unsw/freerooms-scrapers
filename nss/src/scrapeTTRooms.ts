
import fs from "fs";
import fetchXlsx from "./fetchXlsx";
// import decodeXlsx from "./decodeXlsx";
// import { BookingsExcelRow, RawRoomBooking, UngroupedRoomBooking } from "./types";


const TT_URL = "https://publish.unsw.edu.au/timetables?date=2025-11-10&view=week&timetableTypeSelected=1e042cb1-547d-41d4-ae93-a1f2c3d34538&selections=1e042cb1-547d-41d4-ae93-a1f2c3d34538__"


const BATCH_SIZE = 20;

const scrapeTTRooms = async () => {
    const sessionIdentitiesRaw = fs.readFileSync("../sessionIdentities.json", "utf8");
    const sessionIdentities = JSON.parse(sessionIdentitiesRaw);

    // Process batches of 20 rooms
    for (let i = 0; i < 60; i += BATCH_SIZE) {
        const batch = sessionIdentities.slice(i, i + BATCH_SIZE);
        const url = TT_URL + batch.map((s: { id: string; sessionIdentity: string }) => s.sessionIdentity).join("_");
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