# nss-scraper
Scraper for https://nss.cse.unsw.edu.au/tt/. This scrapes the data for all buildings and rooms managed by the UNSW central timetabling system, as well as all bookings on this system. This includes classes and society bookings as well as others.

## Data Sources

Building IDs and names are scraped from the dropdown list of buildings on https://nss.cse.unsw.edu.au/tt/view_multirooms.php?dbafile=2024-KENS-COFA.DBA&campus=KENS. The coordinates and aliases of buildings are manually supplied in `buildingOverrides.json`.

Room data is all scraped from doing a search on https://nss.cse.unsw.edu.au/tt/find_rooms.php?dbafile=2024-KENS-COFA.DBA&campus=KENS. To get all rooms to show up, set the search parameter to be all days and set the start/end time to be equal.

For each room, the facilities are scraped from https://nss.cse.unsw.edu.au/tt/find_rooms.php?dbafile=2024-KENS-COFA.DBA&campus=KENS (same link as above). For the facilities to appear, you need to pass in `show: "show_facilities"` and `room: roomId` in the request body.

Bookings are scraped separately for each room from the individual room pages (e.g. https://nss.cse.unsw.edu.au/tt/view_rooms.php?dbafile=2024-KENS-COFA.DBA&campus=KENS). By setting the date range to be the whole year, each booking will show a bit string on hover (HTML `title` element) describing which weeks of the year it runs in.

Some buildings and rooms are ignored, which can be seen and configured in `src/exclusions.ts`.