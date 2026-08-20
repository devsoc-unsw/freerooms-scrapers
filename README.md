# Freerooms Scrapers

This repository contains the scrapers for the underlying buildings, rooms and bookings data used by Freerooms.

For documentation on the internals of each scraper, please see the READMEs in the respective directories for each scraper. The current scraper is the C++ [`publish-scraper`](./publish-scraper/README.md); the `nss` scraper is legacy.

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
| `abbr`           | Shortened room name.                              | "BrassME305"         |
| `usage`          | Room type.                                        | "CMLB"               |
| `capacity`       | Number of people the room is suitable for.        | 36                   |
| `school`         | School that manages the room.                     | "CSE"                |
| `buildingId`     | ID of the building that contains the room.        | "K-J17"              |
| `floor`          | Floor type.                                       | "Tiered"             |
| `seating`        | Seating type.                                     | "Movable"            |
| `microphone`     | Microphone facilities.                            | ["Lectern (fixed)"]  |
| `accessibility`  | Accessibility facilities.                         | ["Hearing loop"]     |
| `audiovisual`    | Audiovisual facilities.                           | ["Television monitor"] |
| `infotechnology` | Information technology facilities.                | ["IT Lectern"]       |
| `writingMedia`   | Writing facilities.                               | ["Blackboard"]       |
| `service`        | Other room services.                              | ["Break out rooms"]  |
| `lat`            | Latitude of the room.                             | -33.91700            |
| `long`           | Longitude of the room.                            | 151.227791           |
| `imageUrl`       | Room image URL, if available.                     | "https://..."         |

Floor type can be `Flat`, `Tiered`, `Other` or null. Seating type can be `Movable`, `Fixed` or null.

Mapping of room usages can be found [here](https://github.com/devsoc-unsw/freerooms/blob/dev/common/roomUsages.ts).
Mapping of school codes can be found [here](https://github.com/devsoc-unsw/freerooms/blob/dev/common/schools.ts).

### Bookings

| **Field**       | **Description**                                      | **Example**                 |
| --------------- | ---------------------------------------------------- | --------------------------- |
| `roomId`        | ID of the room the booking is for.                   | "K-E19-G05"                 |
| `occurrenceId`  | ID of this exact booking occurrence.                 | "..."                       |
| `eventId`       | ID of the parent Publish event.                      | "..."                       |
| `bookingType`   | Normalised booking type.                             | "LECTURE"                   |
| `name`          | Display name for the booking.                        | "COMP1511"                  |
| `rawName`       | Original booking name from Publish.                  | "COMP1511..."               |
| `eventType`     | Original Publish event type.                         | "..."                       |
| `start`         | Start time of the booking.                           | "2026-01-27T04:00:00+00:00" |
| `end`           | End time of the booking.                             | "2026-01-27T08:00:00+00:00" |
| `plannedSize`   | Planned attendance, if provided.                     | 120                         |
| `source`        | Publish source value, if provided.                   | "..."                       |
| `lastModified`  | Last modified time from Publish, if provided.        | "2026-01-20T00:00:00+00:00" |

Booking types are: `LECTURE`, `TUTORIAL`, `LABORATORY`, `TUTORIAL_LABORATORY`, `WORKSHOP`, `SEMINAR`, `STUDIO`, `CLASS`, `EXAMS`, `SOCIETY`, `INTERNAL`, `BLOCK`, `MISC` and `OTHER`.

### BookingModules

| **Field**      | **Description**                                | **Example** |
| -------------- | ---------------------------------------------- | ----------- |
| `roomId`       | Room for the booking occurrence.               | "K-E19-G05" |
| `occurrenceId` | Booking occurrence the module belongs to.      | "..."       |
| `moduleIndex`  | Position of the module on the booking.          | 0           |
| `code`         | Module/course code.                            | "COMP1511"  |
| `name`         | Module/course name.                            | "Programming Fundamentals" |
| `term`         | Term, if provided.                             | "T1"        |
| `career`       | Career, if provided.                           | "UGRD"      |

### Relationships

The following relationships exist between tables and are tracked by Hasura:

- Every **building** contains 0 or more **rooms**
- Every **room** belongs to a **building** and has 0 or more **bookings**
- Every **booking** belongs to a **room** and has 0 or more **booking modules**

## Making Changes

### Schema updates

To update the active schema:

- Update the relevant `up.sql` and `down.sql` files in `publish-scraper/sql/`
- Update the scraper to produce the matching data
- Update this README if the exposed schema changes

### Adding additional scrapers

To add an additional scraper:

- Create a new subdirectory with the scraper inside it
- Add its tests/build to the GitHub workflow
- Document it in its own README

### Testing

See [`publish-scraper/README.md`](./publish-scraper/README.md) for build, lint and test commands.
