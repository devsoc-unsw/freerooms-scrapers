import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";

const GRAPHQL_URL =
  process.env.GRAPHQL_URL ?? "http://localhost:8080/v1/graphql";
const HASURA_GRAPHQL_ADMIN_SECRET =
  process.env.HASURA_GRAPHQL_ADMIN_SECRET ?? "hasurasecret";

const YEAR = process.env.YEAR
  ? Number.parseInt(process.env.YEAR, 10)
  : new Date().getFullYear();

type Booking = {
  name: string;
  bookingType: string | null;
  start: string;
  end: string;
};

type RoomWithBookings = {
  id: string;
  name: string;
  bookings: Booking[];
};

type Building = {
  id: string;
  name: string;
  lat: number;
  long: number;
  aliases: string[];
  rooms: Array<{
    id: string;
    name: string;
    abbr: string;
    school: string;
    usage: string;
    capacity: number;
  }>;
};

type RoomUtilities = {
  id: string;
  name: string;
  floor: string | null;
  seating: string | null;
  microphone: string[];
  accessibility: string[];
  audiovisual: string[];
  infotechnology: string[];
  writingMedia: string[];
  service: string[];
};

const BOOKINGS_IN_RANGE = `
  query BookingsInRange($start: timestamptz, $end: timestamptz) {
    rooms {
      id
      name
      bookings(
        where: {start: {_lte: $end}, end: {_gte: $start}}
        order_by: {start: asc}
      ) {
        name
        bookingType
        start
        end
      }
    }
  }
`;

const BUILDINGS_AND_ROOMS = `
  query BuildingAndRooms {
    buildings(order_by: {name: asc}) {
      id
      name
      lat
      long
      aliases
      rooms(order_by: {id: asc}) {
        id
        name
        abbr
        school
        usage
        capacity
      }
    }
  }
`;

const BOOKINGS_FOR_ROOM = `
  query BookingsForRoom($roomId: String!) {
    rooms_by_pk(id: $roomId) {
      id
      name
      bookings(order_by: {start: asc}) {
        name
        bookingType
        start
        end
      }
    }
  }
`;

const ROOM_UTILITIES = `
  query RoomUtilities($roomId: String!) {
    rooms_by_pk(id: $roomId) {
      id
      name
      floor
      seating
      microphone
      accessibility
      audiovisual
      infotechnology
      writingMedia
      service
    }
  }
`;

const FIRST_BOOKING = `
  query FirstBooking($start: timestamptz!, $end: timestamptz!) {
    bookings(
      where: {start: {_lt: $end}, end: {_gt: $start}}
      order_by: [{start: asc}, {roomId: asc}]
      limit: 1
    ) {
      roomId
      start
      end
    }
  }
`;

const graphqlRequest = async <T>(
  query: string,
  variables: Record<string, unknown> = {}
): Promise<T> => {
  const response = await fetch(GRAPHQL_URL, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "x-hasura-admin-secret": HASURA_GRAPHQL_ADMIN_SECRET,
    },
    body: JSON.stringify({ query, variables }),
  });

  assert.equal(
    response.ok,
    true,
    `GraphQL HTTP request failed: ${response.status} ${response.statusText}`
  );

  const body = (await response.json()) as {
    data?: T;
    errors?: Array<{ message: string }>;
  };

  assert.equal(
    body.errors,
    undefined,
    `GraphQL returned errors: ${JSON.stringify(body.errors)}`
  );
  assert.notEqual(body.data, undefined, "GraphQL response did not contain data");

  return body.data as T;
};

const assertAscending = <T>(
  values: T[],
  compare: (left: T, right: T) => number,
  description: string
) => {
  for (let i = 1; i < values.length; i += 1) {
    assert.ok(
      compare(values[i - 1], values[i]) <= 0,
      `${description} is not ascending at indexes ${i - 1} and ${i}`
    );
  }
};

const assertBookingShape = (booking: Booking) => {
  assert.equal(typeof booking.name, "string");
  assert.ok(
    booking.bookingType === null || typeof booking.bookingType === "string"
  );
  assert.equal(Number.isNaN(Date.parse(booking.start)), false);
  assert.equal(Number.isNaN(Date.parse(booking.end)), false);
  assert.ok(
    new Date(booking.start).getTime() <= new Date(booking.end).getTime(),
    `Booking starts after it ends: ${JSON.stringify(booking)}`
  );
};

const yearStart = new Date(Date.UTC(YEAR, 0, 1));
const nextYearStart = new Date(Date.UTC(YEAR + 1, 0, 1));

const getFirstBooking = async () => {
  const data = await graphqlRequest<{
    bookings: Array<{ roomId: string; start: string; end: string }>;
  }>(FIRST_BOOKING, {
    start: yearStart.toISOString(),
    end: nextYearStart.toISOString(),
  });

  assert.ok(
    data.bookings.length > 0,
    `The scraper inserted no bookings for ${YEAR}; cannot validate booking queries`
  );

  return data.bookings[0];
};

test("queryBuildingsAndRooms returns all scraped buildings and rooms in the expected order", async () => {
  const data = await graphqlRequest<{ buildings: Building[] }>(
    BUILDINGS_AND_ROOMS
  );

  const buildingsPath = path.resolve(__dirname, "../nss_data/buildings.json");
  const roomsPath = path.resolve(__dirname, "../nss_data/rooms.json");
  const expectedBuildings = JSON.parse(
    fs.readFileSync(buildingsPath, "utf8")
  ) as Array<{ id: string }>;
  const expectedRooms = JSON.parse(fs.readFileSync(roomsPath, "utf8")) as Array<{
    id: string;
  }>;
  const expectedBuildingIds = new Set(
    expectedBuildings
      .filter((building) =>
        expectedRooms.some((room) => room.id.startsWith(`${building.id}-`))
      )
      .map((building) => building.id)
  );

  assert.equal(data.buildings.length, expectedBuildingIds.size);
  assertAscending(
    data.buildings,
    (left, right) => left.name.localeCompare(right.name),
    "Buildings"
  );

  const returnedRoomIds = new Set<string>();
  for (const building of data.buildings) {
    assert.ok(expectedBuildingIds.has(building.id));
    assert.equal(typeof building.name, "string");
    assert.equal(typeof building.lat, "number");
    assert.equal(typeof building.long, "number");
    assert.ok(Array.isArray(building.aliases));

    assertAscending(
      building.rooms,
      (left, right) => left.id.localeCompare(right.id),
      `Rooms in ${building.id}`
    );

    for (const room of building.rooms) {
      returnedRoomIds.add(room.id);
      assert.ok(room.id.startsWith(`${building.id}-`));
      assert.equal(typeof room.name, "string");
      assert.equal(typeof room.abbr, "string");
      assert.equal(typeof room.school, "string");
      assert.equal(typeof room.usage, "string");
      assert.equal(typeof room.capacity, "number");
    }
  }

  assert.equal(returnedRoomIds.size, expectedRooms.length);
  for (const room of expectedRooms) {
    assert.ok(returnedRoomIds.has(room.id), `Missing scraped room ${room.id}`);
  }
});

test("queryBookingsInRange returns only overlapping bookings for the scraped year", async () => {
  const data = await graphqlRequest<{ rooms: RoomWithBookings[] }>(
    BOOKINGS_IN_RANGE,
    {
      start: yearStart.toISOString(),
      end: nextYearStart.toISOString(),
    }
  );

  let bookingCount = 0;
  for (const room of data.rooms) {
    assertAscending(
      room.bookings,
      (left, right) => Date.parse(left.start) - Date.parse(right.start),
      `Bookings for ${room.id}`
    );

    for (const booking of room.bookings) {
      bookingCount += 1;
      assertBookingShape(booking);
      assert.ok(Date.parse(booking.start) <= nextYearStart.getTime());
      assert.ok(Date.parse(booking.end) >= yearStart.getTime());
    }
  }

  assert.ok(bookingCount > 0, `No bookings were returned for ${YEAR}`);
});

test("queryBookingsInRange is inclusive at booking start and end boundaries", async () => {
  const target = await getFirstBooking();

  for (const boundary of [target.start, target.end]) {
    const data = await graphqlRequest<{ rooms: RoomWithBookings[] }>(
      BOOKINGS_IN_RANGE,
      { start: boundary, end: boundary }
    );

    const room = data.rooms.find((candidate) => candidate.id === target.roomId);
    assert.notEqual(room, undefined, `Room ${target.roomId} was not returned`);
    assert.ok(
      room!.bookings.some(
        (booking) => booking.start === target.start && booking.end === target.end
      ),
      `Expected booking was not returned at boundary ${boundary}`
    );
  }
});

test("queryBookingsInRange returns no bookings for a range outside the fresh scraper data", async () => {
  const start = new Date(Date.UTC(YEAR - 20, 0, 1));
  const end = new Date(Date.UTC(YEAR - 20, 0, 2));

  const data = await graphqlRequest<{ rooms: RoomWithBookings[] }>(
    BOOKINGS_IN_RANGE,
    { start: start.toISOString(), end: end.toISOString() }
  );

  assert.ok(data.rooms.length > 0, "Expected rooms to still be returned");
  assert.ok(
    data.rooms.every((room) => room.bookings.length === 0),
    "Found a booking in an out-of-range historical window"
  );
});

test("queryBookingsForRoom returns one valid room with bookings sorted by start", async () => {
  const target = await getFirstBooking();
  const data = await graphqlRequest<{ rooms_by_pk: RoomWithBookings | null }>(
    BOOKINGS_FOR_ROOM,
    { roomId: target.roomId }
  );

  assert.notEqual(data.rooms_by_pk, null);
  assert.equal(data.rooms_by_pk!.id, target.roomId);
  assert.equal(typeof data.rooms_by_pk!.name, "string");
  assert.ok(data.rooms_by_pk!.bookings.length > 0);
  assertAscending(
    data.rooms_by_pk!.bookings,
    (left, right) => Date.parse(left.start) - Date.parse(right.start),
    `Bookings for ${target.roomId}`
  );
  data.rooms_by_pk!.bookings.forEach(assertBookingShape);
});

test("queryBookingsForRoom returns null for an unknown room id", async () => {
  const data = await graphqlRequest<{ rooms_by_pk: RoomWithBookings | null }>(
    BOOKINGS_FOR_ROOM,
    { roomId: "CI-NOT-A-REAL-ROOM" }
  );

  assert.equal(data.rooms_by_pk, null);
});

test("queryRoomUtilities returns the utility fields inserted by the scraper", async () => {
  const roomsPath = path.resolve(__dirname, "../nss_data/rooms.json");
  const facilitiesPath = path.resolve(__dirname, "../nss_data/facilities.json");
  const expectedRooms = JSON.parse(fs.readFileSync(roomsPath, "utf8")) as Array<{
    id: string;
    name: string;
  }>;
  const expectedFacilities = JSON.parse(
    fs.readFileSync(facilitiesPath, "utf8")
  ) as Array<Omit<RoomUtilities, "id" | "name">>;

  assert.ok(expectedRooms.length > 0, "No rooms exist in nss_data/rooms.json");
  assert.equal(expectedFacilities.length, expectedRooms.length);

  const expected = {
    id: expectedRooms[0].id,
    name: expectedRooms[0].name,
    ...expectedFacilities[0],
  };
  const data = await graphqlRequest<{ rooms_by_pk: RoomUtilities | null }>(
    ROOM_UTILITIES,
    { roomId: expected.id }
  );

  assert.deepEqual(data.rooms_by_pk, expected);
});

test("queryRoomUtilities returns null for an unknown room id", async () => {
  const data = await graphqlRequest<{ rooms_by_pk: RoomUtilities | null }>(
    ROOM_UTILITIES,
    { roomId: "CI-NOT-A-REAL-ROOM" }
  );

  assert.equal(data.rooms_by_pk, null);
});