# libcal-scraper
Scraper for https://unswlibrary-bookings.libcal.com/. This scrapes data on study rooms in the Main Library and Law Library, as well as the bookings for these rooms.

## Data Sources

The booking data is found on this page:
https://unswlibrary-bookings.libcal.com/r/new/availability?lid=6581&zone=0&gid=0&capacity=0

However, the data is not scraped from the HTML, but instead fetched by mimicking the API call made by this page to https://unswlibrary-bookings.libcal.com/spaces/availability/grid. This returns a list of "slots" with item (room) IDs and start/end times, some of which are booked.

The list of item IDs from the above JSON is then used to scrape the data for each room from the individual room pages (e.g. https://unswlibrary-bookings.libcal.com/space/46481). This is scraped from the HTML using [cheerio](https://cheerio.js.org/).
