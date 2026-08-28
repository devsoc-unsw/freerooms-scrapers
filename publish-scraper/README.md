# publish-scraper

C++ scraper for the UNSW Publish public API. It provides the buildings, rooms and bookings used by Freerooms and replaces the legacy `nss` scraper.

## Data Sources

Building, room and facility data is stored in `data/buildings.json`, `data/rooms.json` and `data/facilities.json`. This static data was carried forward from the old NSS scraper and remains the authoritative source for room metadata such as capacity, usage, school, coordinates and facilities.

`data/rooms.json` also stores each room's Publish UUID in `publishId`. The separate `rooms` command reconciles those UUIDs against the current Publish location list and updates matched mappings without deleting or inventing static room metadata.

Bookings are scraped from the UNSW Publish public API at `https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public`. Publish event data is also used to obtain room image links where available.

Buildings, rooms and Publish locations that should not be exposed are configured in `config/exclusions.json`. Exclusions are applied both to the normal scrape and to Publish room reconciliation. Supported exclusion sets are:

- `buildingIds`
- `roomIds`
- `virtualLocationIds`
- `usages`
- `schools`

Usage and school exclusions are applied using the committed static room metadata. Publish locations that do not correspond to an existing static room cannot be classified by usage or school because Publish's location list does not provide those fields.

## Packages

| **Package** | **Purpose** |
| ----------- | ----------- |
| C++20 compiler | Compile the scraper |
| CMake 3.20+ | Configure and build the project |
| libcurl | HTTP requests |
| nlohmann/json 3.11.3 | JSON parsing; fetched automatically if unavailable |
| Catch2 3.8.1 | Unit/integration tests; fetched automatically if unavailable |
| clang-format | Code formatting |
| clang-tidy | Static analysis/linting |
| Docker | Optional container build/run |

On Ubuntu/Debian, the system dependencies can be installed with:

```bash
sudo apt-get install build-essential cmake libcurl4-openssl-dev clang-format clang-tidy ninja-build
```

On Fedora:

```bash
sudo dnf install gcc-c++ cmake libcurl-devel clang-tools-extra ninja-build
```

## Commands

Run these from `publish-scraper/` unless stated otherwise.

| **Task** | **Command** |
| -------- | ----------- |
| Configure | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` |
| Build | `cmake --build build` |
| Run scraper | `HASURAGRES_URL=http://localhost:8000 HASURAGRES_API_KEY=my_key ./build/publish-scraper` |
| Run scraper explicitly | `HASURAGRES_URL=http://localhost:8000 HASURAGRES_API_KEY=my_key ./build/publish-scraper scrape` |
| Run a specific year | `YEAR=2026 HASURAGRES_URL=http://localhost:8000 HASURAGRES_API_KEY=my_key ./build/publish-scraper` |
| Reconcile/update room Publish IDs | `./build/publish-scraper rooms` |
| Format | `./scripts/format.sh` |
| Lint | `./scripts/lint.sh` |
| Unit tests | `ctest --test-dir build -L unit --output-on-failure` |
| Integration tests | `YEAR=2026 GRAPHQL_URL=http://localhost:8080/v1/graphql ctest --test-dir build -L integration --output-on-failure` |
| Docker build | From repo root: `docker build -f publish-scraper.dockerfile -t publish-scraper:local .` |

### Room reconciliation

`./build/publish-scraper rooms` performs a maintenance-only room mapping pass:

1. Load and validate the committed static room data.
2. Fetch the current Publish location list.
3. Apply `config/exclusions.json`.
4. Match Publish location names to existing Freerooms room IDs.
5. Refuse to write if Publish contains duplicate mappings for the same room ID.
6. Update `publishId` for matched entries in `data/rooms.json`.
7. Report static rooms missing from Publish, Publish rooms missing from the static data, and unrecognised Publish locations.

Unmatched static rooms are deliberately left unchanged for manual review. The command does not add/remove rooms or regenerate building/facility metadata because Publish does not contain all of the metadata required by Freerooms.

If this command is run inside Docker and the updated JSON needs to persist on the host, mount the `data` directory as a writable volume.

## Environment Variables

| **Variable** | **Required** | **Default** | **Description** |
| ------------ | ------------ | ----------- | --------------- |
| `HASURAGRES_URL` | For `scrape` | none | Hasuragres base URL |
| `HASURAGRES_API_KEY` | For `scrape` | none | Hasuragres API key |
| `YEAR` | No | current Sydney year | Booking year to scrape |
| `PUBLISH_MAX_CONCURRENT_REQUESTS` | No | `4` | Maximum concurrent Publish event batch requests |
| `PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS` | No | `0` | Minimum delay in milliseconds between the start of Publish HTTP requests, including retries |
| `GRAPHQL_URL` | Integration tests only | `http://localhost:8080/v1/graphql` | GraphQL endpoint tested by the integration suite |

The Publish category selection limit remains fixed at 20 locations per event request because this is the request batching limit, not a rate-control setting.

## Testing Against the Local GraphQL API

The integration tests make real HTTP queries to the local GraphQL API, matching the style of the legacy NSS tests while also covering the additional Publish schema.

A typical local flow is:

```bash
# In the graphql-api repository
API_KEYS=ci_key docker compose up --build --detach --wait

# In publish-scraper/
YEAR=2026 \
HASURAGRES_URL=http://localhost:8000 \
HASURAGRES_API_KEY=ci_key \
./build/publish-scraper

YEAR=2026 \
GRAPHQL_URL=http://localhost:8080/v1/graphql \
ctest --test-dir build -L integration --output-on-failure
```

The integration suite checks:

- every active building and room is exposed and related correctly;
- room ordering and building membership;
- booking overlap/range queries and boundary behaviour;
- booking ordering by start time;
- unknown-room behaviour;
- exact room facility fields;
- room image URLs;
- detailed Publish booking fields and room relationships; and
- `BookingModules` relationships in both directions.

The API must already be running and populated by a successful scraper run before the integration tests are executed.
