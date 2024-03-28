import { load } from 'cheerio';
import { RoomBooking, Room } from "./types";
import toSydneyTime from "./toSydneyTime";
import axios from "axios";
import * as fs from 'fs';
import { DRYRUN, HASURAGRES_API_KEY, HASURAGRES_URL } from './config';

const ROOM_URL = "https://unswlibrary-bookings.libcal.com/space/";
const BOOKINGS_URL = "https://unswlibrary-bookings.libcal.com/spaces/availability/grid";
const LIBRARIES = [
    { name: 'Main Library', libcalCode: '6581', buildingId: 'K-F21' },
    { name: 'Law Library', libcalCode: '6584', buildingId: 'K-F8' },
];

const scrapeLibrary = async (library: typeof LIBRARIES[number]) => {
    const response = await downloadBookingsPage(library.libcalCode);
    const bookingData = parseBookingData(response.data['slots']);

    const allRoomData: Room[] = [];
    const allRoomBookings: RoomBooking[] = [];

    for (const roomID in bookingData) {
        let roomData: Room | null = null;
        try {
            roomData = await getRoomData(roomID, library.buildingId);
        } catch (e) {
            console.warn(`Failed to scrape room ${roomID} in ${library.buildingId}`);
        }

        if (!roomData) continue; // skipping non-rooms
        allRoomData.push(roomData);

        let i = 0;
        while (i < bookingData[roomID].length) {
            const currBooking: RoomBooking = {
                bookingType: 'LIB',
                name: "Library Booking",
                roomId: roomData.id,
                start: bookingData[roomID][i].start,
                end: bookingData[roomID][i].end,
            }
            i++;

            // Combine all subsequent bookings that start when this ends
            while (i < bookingData[roomID].length &&
                   bookingData[roomID][i].start.getTime() == currBooking.end.getTime()) {
                currBooking.end = bookingData[roomID][i].end;
                i++;
            }

            allRoomBookings.push(currBooking);
        }
    }

    return { rooms: allRoomData, bookings: allRoomBookings };
}

// Formats a date into YYYY-MM-DD format
const formatDate = (date: Date): string => {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');

    return `${year}-${month}-${day}`;
}

const downloadBookingsPage = async(locationId: string) => {

    const todaysDate = formatDate(new Date());

    // Furthest seems to be 2 weeks in the future
    const twoWeeks = new Date();
    twoWeeks.setDate(twoWeeks.getDate() + 14);
    const furthestBookableDate = formatDate(twoWeeks);

    const postData = {
        lid: locationId,
        gid: '0',
        eid: '-1',
        seat: '0',
        seatId: '0',
        zone: '0',
        start: todaysDate,
        end: furthestBookableDate,
        pageIndex: '0',
        pageSize: '18'
    };

    const headers = {
        'Content-Type': 'application/x-www-form-urlencoded', // because the request data is URL encoded
        'Referer': 'https://unswlibrary-bookings.libcal.com/'
    };

    return await axios.post(BOOKINGS_URL, new URLSearchParams(postData), { headers });
}

interface ResponseData {
    start: string,
    end: string,
    itemId: number,
    checksum: string,
    className?: string
}

const parseBookingData = (bookingData: ResponseData[]) => {

    const bookings: { [roomNumber: number]: { start: Date, end: Date }[] } = {};

    for (const slot of bookingData) {
        if (!(slot.itemId in bookings)) {
            bookings[slot.itemId] = [];
        }

        if (slot.className == "s-lc-eq-checkout") {
            bookings[slot.itemId].push(
                {
                    start: toSydneyTime(new Date(slot.start)),
                    end: toSydneyTime(new Date(slot.end)),
                }
            )
        }
    }

    return bookings;
}

const getRoomData = async (roomId: string, buildingId: string) => {

    const response = await axios.get(ROOM_URL + roomId, {});
    const $ = load(response.data);

    const $heading = $('h1#s-lc-public-header-title');

    // Remove whitespace and split the name, location and capacity into newlines
    const data = $heading.text().trim().split(/\s{2,}/g);
    const [name, rawLocation, rawCapacity] = data;

    // We only care about rooms and pods
    if (!name.match(/RM|POD/)) {
        return null;
    }

    const libraryName = rawLocation.replace(/[()]/, '').split(':')[0];
    const capacity = parseInt(rawCapacity.split(": ")[1]);
    let roomNumber = name.split(' ')[2];
    if (name.match(/POD/)) {
        // Pods are just numbered 1-8 so prepend POD
        roomNumber = 'POD' + roomNumber;
    }

    const roomData: Room = {
        name: libraryName + ' ' + name,
        abbr: name,
        id: buildingId + "-" + roomNumber,
        usage: "LIB",
        capacity,
        school: " ",
        buildingId: buildingId
    }

    return roomData;
}

const runScrapeJob = async () => {
    console.time('Scraping');
    const allRooms: Room[] = [];
    const allBookings: RoomBooking[] = [];
    for (const library of LIBRARIES) {
        const { rooms, bookings } = await scrapeLibrary(library);
        allRooms.push(...rooms);
        allBookings.push(...bookings);
    }
    console.timeEnd('Scraping');

    // Send to Hasuragres
    const requestConfig = {
        headers: {
            "Content-Type": "application/json",
            "X-API-Key": HASURAGRES_API_KEY,
        }
    }

    await axios.post(
      `${HASURAGRES_URL}/insert`,
      {
          metadata: {
              table_name: "Rooms",
              columns: ["abbr", "name", "id", "usage", "capacity", "school", "buildingId"],
              sql_up: fs.readFileSync("./sql/rooms/up.sql", "utf8"),
              sql_down: fs.readFileSync("./sql/rooms/down.sql", "utf8"),
              // overwrite all outdated lib rooms
              sql_before: "DELETE FROM Rooms WHERE \"usage\" = 'LIB' " +
                          `AND "id" NOT IN (${allRooms.map(room => `'${room.id}'`).join(",")});`,
              write_mode: 'append',
              dryrun: DRYRUN,
          },
          payload: allRooms
      },
      requestConfig
    );

    // libcal shows all bookings that start during or after the current 30-min period
    const baseTime = new Date();
    baseTime.setMinutes(baseTime.getMinutes() < 30 ? 0 : 30, 0, 0);
    await axios.post(
      `${HASURAGRES_URL}/insert`,
      {
          metadata: {
              table_name: "Bookings",
              columns: ["bookingType", "name", "roomId", "start", "end"],
              sql_up: fs.readFileSync("./sql/bookings/up.sql", "utf8"),
              sql_down: fs.readFileSync("./sql/bookings/down.sql", "utf8"),
              sql_before: fs.readFileSync("./sql/bookings/before.sql", "utf8"),
              sql_after: fs.readFileSync("./sql/bookings/after.sql", "utf8"),
              write_mode: 'append',
              dryrun: DRYRUN,
          },
          payload: allBookings
      },
      requestConfig
    );
}

runScrapeJob();
