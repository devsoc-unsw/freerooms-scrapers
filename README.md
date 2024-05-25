# Freerooms Scrapers

This repository contains the scrapers for the underlying buildings, rooms and bookings data used by Freerooms.

For documentation on the internals of each scraper, please see the READMEs in the respective directories for each scraper.

For instructions on how you can access this data, see the [DevSoc GraphQL API](https://github.com/devsoc-unsw/graphql-api).

## Schema

### Buildings

| **Field** | **Description**                              | **Example**     |
| --------- | -------------------------------------------- | --------------- |
| `id`      | Building ID in the format `CAMPUS-GRID_REF`. | "K-F8"          |
| `name`    | Name of the building.                        | "Law Building"  |
| `lat`     | Latitude of the building.                    | -33.91700       |
| `long`    | Longitude of the building.                   | 151.227791      |
| `aliases` | List of alternative names for the building.  | ["Law Library"] |

### Rooms

| **Field**        | **Description**                                   | **Example**          |
| ---------------- | ------------------------------------------------- | -------------------- |
| `id`             | Room ID in the format `CAMPUS-GRID_REF-ROOM_NUM`. | "K-J17-305"          |
| `name`           | Name of the room.                                 | "Brass Lab J17 305"  |
| `abbr`           | Shortened name, as seen on timetable.             | "BrassME305"         |
| `usage`          | Room type - see below for list.                   | "CMLB"               |
| `capacity`       | Number of people the room is suitable for.        | 36                   |
| `school`         | School that manages the room - `" "` if none.     | "CSE"                |
| `buildingId`     | ID of building that room is in.                   | "K-J17"              |
| `floor`          | Floor type - see below for list.                  | "Tiered"             |
| `seating`        | Seating type - see below for list.                | "Movable"            |
| `microphone`     | List of microphone facilities of the room.        | "Lectern (fixed)"    |
| `accessibility`  | List of accessibility facilities of the room.     | "Hearing loop"       |
| `audiovisual`    | List of audiovisual facilities of the room.       | "Television monitor" |
| `infotechnology` | List of intotechnology facilities of the room.    | "IT Lectern"         |
| `writingMedia`   | List of writingMedia facilities of the room.      | "Blackboard"         |
| `service`        | List of service facilities of the room.           | "Break out rooms"    |

Floor type can be 'Flat', 'Tiered', 'Other' or 'Unknown'.
Seating type can be 'Movable', 'Fixed' or 'Unknown'.

Mapping of room usages can be found [here](https://github.com/devsoc-unsw/freerooms/blob/dev/common/roomUsages.ts).
Mapping of school codes can be found [here](https://github.com/devsoc-unsw/freerooms/blob/dev/common/schools.ts).

### Bookings

| **Field**     | **Description**                                      | **Example**                 |
| ------------- | ---------------------------------------------------- | --------------------------- |
| `bookingType` | Type of booking - see below.                         | "SOCIETY"                   |
| `name`        | Name of the booking (usually related to the booker). | "SOFTWAREDEV"               |
| `roomId`      | ID of the room the booking is for.                   | "K-E19-G05"                 |
| `start`       | Start time of the booking.                           | "2024-01-27T04:00:00+00:00" |
| `end`         | End time of the booking.                             | "2024-01-27T08:00:00+00:00" |

Full list of current booking types is: "CLASS", "SOCIETY", "INTERNAL", "LIB", "BLOCK", "MISC".

### Relationships

The following relationships exist between tables. These relationships are tracked by Hasura and can be followed in GraphQL queries.

- Every **building** contains 1 or more **rooms**
- Every **room** belongs to a **building**
- Every **room** has 0 or more **bookings**
- Every **booking** is for a specific **room**

## Making Changes

### Schema updates

To update the schema, you will need to:

- Update the relevant `up.sql` and `down.sql` files in the root `sql/` directory
- Update the scrapers to produce this data

### Adding additional scrapers

To add additional scrapers, you will need to:

- Create a new subdirectory with the scraper inside it
- Ensure that if you are using the schema SQL files, you reference them using symlinks so all scrapers are updated
- Add to the GitHub workflow so that it also tests/builds/deploys the new scraper

### Testing

See the [DevSoc GraphQL API docs](https://github.com/devsoc-unsw/graphql-api/blob/master/scrapers.md) on how to test scrapers.
