#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/sort.hpp"
#include "bookings/transform.hpp"
#include "config/exclusions.hpp"
#include "data/room_metadata.hpp"
#include "data/static_data.hpp"
#include "database/client.hpp"
#include "database/request.hpp"
#include "database/static_json.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "publish/settings.hpp"
#include "rooms/exclusions.hpp"
#include "rooms/image_url.hpp"
#include "rooms/publish_mapping.hpp"

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct YearConfig {
    int value = 0;
    bool overridden = false;
};

enum class Command {
    Scrape,
    Rooms,
};

double elapsed_seconds(const Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

void print_elapsed(std::ostream& output, const double seconds) {
    output << std::fixed << std::setprecision(2) << seconds << "s";
}

template <typename Function> auto timed_step(const std::string_view name, Function&& function) {
    std::cout << name << "...\n";

    const auto start = Clock::now();

    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Function>>) {
            std::forward<Function>(function)();

            std::cout << "  Completed in ";

            print_elapsed(std::cout, elapsed_seconds(start));

            std::cout << "\n\n";
        } else {
            auto result = std::forward<Function>(function)();

            std::cout << "  Completed in ";

            print_elapsed(std::cout, elapsed_seconds(start));

            std::cout << "\n\n";

            return result;
        }
    } catch (...) {
        std::cerr << "  Failed after ";

        print_elapsed(std::cerr, elapsed_seconds(start));

        std::cerr << '\n';

        throw;
    }
}

int current_sydney_year() {
    using namespace std::chrono;

    // Sydney observes UTC+11 around New Year, which is the only boundary that
    // matters when determining the current Sydney calendar year.
    const auto sydney_time = system_clock::now() + hours{11};

    const auto time = system_clock::to_time_t(sydney_time);

    const auto* calendar_time = std::gmtime(&time);

    if (calendar_time == nullptr) {
        throw std::runtime_error{"Could not determine current year"};
    }

    return calendar_time->tm_year + 1900;
}

YearConfig load_year() {
    const auto* value = std::getenv("YEAR");

    if (value == nullptr || *value == '\0') {
        return YearConfig{
            .value = current_sydney_year(),
            .overridden = false,
        };
    }

    const std::string year_string{value};

    int year = 0;

    const auto result =
        std::from_chars(year_string.data(), year_string.data() + year_string.size(), year);

    if (result.ec != std::errc{} || result.ptr != year_string.data() + year_string.size() ||
        year < 2000 || year > 2100) {
        throw std::runtime_error{"Invalid YEAR value: " + year_string};
    }

    return YearConfig{
        .value = year,
        .overridden = true,
    };
}

Command parse_command(const int argc, char* argv[]) {
    if (argc == 1) {
        return Command::Scrape;
    }

    if (argc != 2) {
        throw std::runtime_error{"Usage: publish-scraper [scrape|rooms]"};
    }

    const std::string_view command{argv[1]};

    if (command == "scrape") {
        return Command::Scrape;
    }

    if (command == "rooms") {
        return Command::Rooms;
    }

    if (command == "--help" || command == "-h") {
        std::cout << "Usage: publish-scraper [scrape|rooms]\n\n"
                  << "  scrape  Scrape bookings and publish all data to Hasuragres (default)\n"
                  << "  rooms   Reconcile Publish room locations and update data/rooms.json\n";
        std::exit(0);
    }

    throw std::runtime_error{"Unknown command: " + std::string{command} +
                             "\nUsage: publish-scraper [scrape|rooms]"};
}

std::size_t count_event_rows(const publish::EventsResponse& events) {
    std::size_t count = 0;

    for (const auto& category : events.category_events) {
        count += category.results.size();
    }

    return count;
}

data::StaticData load_filtered_static_data(const config::Exclusions& exclusions) {
    auto static_data = data::load_static_data("data");

    data::validate_static_data(static_data);

    const auto excluded_room_count =
        rooms::filter_excluded_static_rooms(static_data.rooms, exclusions);

    std::cout << "  Buildings: " << static_data.buildings.size() << '\n'
              << "  Rooms after exclusions: " << static_data.rooms.size() << '\n'
              << "  Excluded static rooms: " << excluded_room_count << "\n\n";

    return static_data;
}

void print_request_settings(const publish::RequestSettings& settings) {
    std::cout << "Publish request limits:\n"
              << "  Maximum concurrent event requests: " << settings.max_concurrent_requests << '\n'
              << "  Minimum time between request starts: "
              << settings.min_time_between_requests.count() << " ms\n\n";
}

int run_rooms_command(const publish::RequestSettings& request_settings) {
    const auto exclusions = timed_step(
        "Loading exclusions", [] { return config::load_exclusions("config/exclusions.json"); });

    auto static_data = timed_step("Loading and validating static room data", [] {
        auto result = data::load_static_data("data");

        data::validate_static_data(result);

        std::cout << "  Buildings: " << result.buildings.size() << '\n'
                  << "  Rooms: " << result.rooms.size() << "\n\n";

        return result;
    });

    http::Client http_client;
    publish::Client publish_client{http_client, request_settings};

    auto locations = timed_step("Fetching Publish room locations",
                                [&] { return publish_client.get_locations(); });

    const auto raw_location_count = locations.size();

    const auto excluded_location_count = timed_step("Applying Publish location exclusions", [&] {
        return rooms::filter_excluded_publish_locations(locations, exclusions, static_data.rooms);
    });

    const auto excluded_static_room_count = timed_step("Applying static room exclusions", [&] {
        return rooms::filter_excluded_static_rooms(static_data.rooms, exclusions);
    });

    std::cout << "  Publish locations returned: " << raw_location_count << '\n'
              << "  Excluded Publish locations: " << excluded_location_count << '\n'
              << "  Locations to reconcile: " << locations.size() << '\n'
              << "  Excluded static rooms: " << excluded_static_room_count << '\n'
              << "  Static rooms to reconcile: " << static_data.rooms.size() << "\n\n";

    const auto report = timed_step("Reconciling Publish locations with rooms.json", [&] {
        return rooms::match_publish_locations(static_data.rooms, locations);
    });

    std::cout << "Room reconciliation:\n"
              << "  Matched rooms: " << report.matches.size() << '\n'
              << "  Static rooms missing from Publish: " << report.missing_from_publish.size()
              << '\n'
              << "  Publish rooms missing from static data: " << report.missing_from_static.size()
              << '\n'
              << "  Unrecognised Publish locations: "
              << report.unrecognised_publish_locations.size() << '\n'
              << "  Duplicate Publish room IDs: " << report.duplicate_publish_room_ids.size()
              << "\n\n";

    if (!report.duplicate_publish_room_ids.empty()) {
        std::cerr << "Duplicate Publish room IDs:\n";

        for (const auto& room_id : report.duplicate_publish_room_ids) {
            std::cerr << "  " << room_id << '\n';
        }

        throw std::runtime_error{"Refusing to update rooms.json while Publish contains duplicate "
                                 "room IDs"};
    }

    if (!report.missing_from_publish.empty()) {
        std::cout << "Static rooms not found in Publish:\n";

        for (const auto& room_id : report.missing_from_publish) {
            std::cout << "  " << room_id << '\n';
        }

        std::cout << '\n';
    }

    if (!report.missing_from_static.empty()) {
        std::cout << "Publish locations not present in data/rooms.json:\n";

        for (const auto& location : report.missing_from_static) {
            std::cout << "  " << location.name << " [" << location.identity << "]\n";
        }

        std::cout << '\n';
    }

    const auto write_result = timed_step("Updating Publish IDs in data/rooms.json", [&] {
        return data::write_publish_ids("data/rooms.json", report.matches);
    });

    std::cout << "Room metadata update complete:\n"
              << "  Matched mappings written: " << write_result.matched_rooms << '\n'
              << "  Publish IDs changed: " << write_result.changed_publish_ids << '\n'
              << "  Unmatched rooms were left unchanged for manual review\n";

    return 0;
}

int run_scrape_command(const publish::RequestSettings& request_settings) {
    const auto year_config = timed_step("Loading scrape configuration", [] { return load_year(); });

    const auto year = year_config.value;

    const auto database_config = timed_step(
        "Loading database configuration", [] { return database::load_config_from_environment(); });

    const auto exclusions = timed_step(
        "Loading exclusions", [] { return config::load_exclusions("config/exclusions.json"); });

    std::cout << "Scrape year: " << year;

    if (year_config.overridden) {
        std::cout << " (YEAR override)";
    } else {
        std::cout << " (automatic Sydney year)";
    }

    std::cout << '\n' << "Database target: " << database_config.base_url << "\n\n";

    auto static_data = timed_step("Loading and validating static room data",
                                  [&] { return load_filtered_static_data(exclusions); });

    http::Client http_client;
    publish::Client publish_client{http_client, request_settings};

    const auto view_options = timed_step("Fetching Publish view options",
                                         [&] { return publish_client.get_view_options(); });

    const auto location_ids = timed_step("Collecting Publish room IDs", [&] {
        std::vector<std::string> result;

        result.reserve(static_data.rooms.size());

        for (const auto& room : static_data.rooms) {
            if (!room.publish_id.has_value()) {
                continue;
            }

            result.push_back(*room.publish_id);
        }

        return result;
    });

    std::cout << "  Rooms with Publish IDs: " << location_ids.size() << "\n\n";

    const auto events = timed_step("Fetching Publish events", [&] {
        return publish_client.get_events(location_ids, view_options, year);
    });

    const auto raw_event_count = count_event_rows(events);

    std::cout << "  Returned room categories: " << events.category_events.size() << '\n'
              << "  Raw room-event rows: " << raw_event_count << "\n\n";

    const auto image_count = timed_step("Extracting Publish room images", [&] {
        return rooms::apply_publish_image_urls(static_data.rooms, events);
    });

    std::cout << "  Rooms with image URLs: " << image_count << "\n\n";

    auto bookings = timed_step("Transforming Publish events into bookings", [&] {
        return bookings::transform_publish_events(events, static_data.rooms);
    });

    if (bookings.size() != raw_event_count) {
        throw std::runtime_error{"Booking transformation changed the number of event rows"};
    }

    const auto removed_count = timed_step("Filtering non-occupancy bookings", [&] {
        return bookings::filter_bookings_for_occupancy(bookings);
    });

    std::cout << "  Removed: " << removed_count << '\n'
              << "  Remaining bookings: " << bookings.size() << "\n\n";

    timed_step("Sorting bookings", [&] { bookings::sort_bookings(bookings); });

    auto building_payload = timed_step("Serializing buildings", [&] {
        return database::serialize_buildings(static_data.buildings, static_data.rooms);
    });

    auto room_payload = timed_step("Serializing rooms",
                                   [&] { return database::serialize_rooms(static_data.rooms); });

    auto booking_payload =
        timed_step("Serializing bookings", [&] { return bookings::serialize_bookings(bookings); });

    auto module_payload = timed_step("Serializing booking modules",
                                     [&] { return bookings::serialize_booking_modules(bookings); });

    const auto building_count = building_payload.size();
    const auto room_count = room_payload.size();
    const auto booking_count = booking_payload.size();
    const auto module_count = module_payload.size();

    auto batch_request = timed_step("Building Hasuragres transaction", [&] {
        return database::build_batch_request(std::move(building_payload),
                                             std::move(room_payload),
                                             std::move(booking_payload),
                                             std::move(module_payload),
                                             year);
    });

    if (!batch_request.is_array() || batch_request.size() != 4) {
        throw std::runtime_error{"Database transaction must contain four table inserts"};
    }

    std::cout << "Database payload:\n"
              << "  Buildings: " << building_count << '\n'
              << "  Rooms: " << room_count << '\n'
              << "  Bookings: " << booking_count << '\n'
              << "  BookingModules: " << module_count << "\n\n";

    database::Client database_client{http_client, database_config};

    const auto insert_result = timed_step("Uploading database transaction", [&] {
        return database_client.batch_insert(batch_request);
    });

    const auto request_mebibytes =
        static_cast<double>(insert_result.request_bytes) / 1024.0 / 1024.0;

    std::cout << "Database insert complete:\n"
              << "  HTTP status: " << insert_result.status_code << '\n'
              << "  Request size: " << request_mebibytes << " MiB\n";

    if (!insert_result.response_body.empty()) {
        std::cout << "  Response: " << insert_result.response_body << '\n';
    }

    return 0;
}

} // namespace

int main(const int argc, char* argv[]) {
    std::cout << std::unitbuf;

    const auto total_start = Clock::now();

    try {
        std::cout << "Publish Scraper\n\n";

        const auto command = parse_command(argc, argv);

        const auto request_settings = timed_step("Loading Publish request settings", [] {
            return publish::load_request_settings_from_environment();
        });

        print_request_settings(request_settings);

        const auto result = command == Command::Rooms ? run_rooms_command(request_settings)
                                                      : run_scrape_command(request_settings);

        std::cout << "\nCommand completed successfully in ";

        print_elapsed(std::cout, elapsed_seconds(total_start));

        std::cout << '\n';

        return result;
    } catch (const std::exception& error) {
        std::cerr << "\nCommand failed after ";

        print_elapsed(std::cerr, elapsed_seconds(total_start));

        std::cerr << "\nError: " << error.what() << '\n';

        return 1;
    }
}
