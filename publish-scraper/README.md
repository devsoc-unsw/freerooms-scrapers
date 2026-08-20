# publish-scraper

C++ scraper for the UNSW Publish public API. It provides the buildings, rooms and bookings used by Freerooms and replaces the legacy `nss` scraper.

## Data Sources

Building, room and facility data is stored in `data/buildings.json`, `data/rooms.json` and `data/facilities.json`. This static data was carried forward from the old NSS scraper and is maintained in the repository. `rooms.json` also stores the Publish ID used to match each room to Publish.

Bookings are scraped from the UNSW Publish public API at `https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public`. Publish is also used to obtain room image links where available.

Buildings, rooms and usages that should not be exposed are configured in `config/exclusions.json`.

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
| Run | `HASURAGRES_URL=http://localhost:8000 HASURAGRES_API_KEY=my_key ./build/publish-scraper` |
| Run a specific year | `YEAR=2026 HASURAGRES_URL=http://localhost:8000 HASURAGRES_API_KEY=my_key ./build/publish-scraper` |
| Format | `./scripts/format.sh` |
| Lint | `./scripts/lint.sh` |
| Unit tests | `ctest --test-dir build -L unit --output-on-failure` |
| Integration tests | `YEAR=2026 GRAPHQL_URL=http://localhost:8080/v1/graphql ctest --test-dir build -L integration --output-on-failure` |
| Docker build | From repo root: `docker build -f publish-scraper.dockerfile -t publish-scraper:local .` |

`HASURAGRES_URL` and `HASURAGRES_API_KEY` are required when running the scraper. `YEAR` is optional and defaults to the current year in Sydney.

Integration tests expect the local GraphQL API to be running and populated by the scraper.
