#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/transform.hpp"
#include "data/static_data.hpp"
#include "database/client.hpp"
#include "database/request.hpp"
#include "database/static_json.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "rooms/image_url.hpp"

#include <charconv>
#include <chrono>
#include <cstdlib>
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

using Clock =
    std::chrono::steady_clock;

double elapsed_seconds(
    const Clock::time_point start
) {
    return std::chrono::duration<double>(
        Clock::now() - start
    ).count();
}

void print_elapsed(
    const double seconds
) {
    std::cout
        << std::fixed
        << std::setprecision(2)
        << seconds
        << "s";
}

template <typename Function>
auto timed_step(
    const std::string_view name,
    Function&& function
) {
    std::cout
        << name
        << "...\n";

    const auto start =
        Clock::now();

    try {
        if constexpr (
            std::is_void_v<
                std::invoke_result_t<Function>
            >
        ) {
            std::forward<Function>(
                function
            )();

            std::cout
                << "  Completed in ";

            print_elapsed(
                elapsed_seconds(start)
            );

            std::cout
                << "\n\n";
        }
        else {
            auto result =
                std::forward<Function>(
                    function
                )();

            std::cout
                << "  Completed in ";

            print_elapsed(
                elapsed_seconds(start)
            );

            std::cout
                << "\n\n";

            return result;
        }
    }
    catch (...) {
        std::cerr
            << "  Failed after ";

        print_elapsed(
            elapsed_seconds(start)
        );

        std::cerr
            << '\n';

        throw;
    }
}

int load_year() {
    const auto* value =
        std::getenv("YEAR");

    if (
        value == nullptr
        || *value == '\0'
    ) {
        throw std::runtime_error{
            "Missing required environment variable: YEAR"
        };
    }

    const std::string year_string{
        value
    };

    int year = 0;

    const auto result =
        std::from_chars(
            year_string.data(),
            year_string.data()
                + year_string.size(),
            year
        );

    if (
        result.ec != std::errc{}
        || result.ptr
            != year_string.data()
                + year_string.size()
        || year < 2000
        || year > 2100
    ) {
        throw std::runtime_error{
            "Invalid YEAR value: "
            + year_string
        };
    }

    return year;
}

std::size_t count_event_rows(
    const publish::EventsResponse& events
) {
    std::size_t count = 0;

    for (
        const auto& category :
        events.category_events
    ) {
        count +=
            category.results.size();
    }

    return count;
}

}

int main() {
    const auto total_start =
        Clock::now();

    try {
        std::cout
            << "Publish Scraper\n\n";

        const auto year =
            timed_step(
                "Loading scrape configuration",
                [] {
                    return load_year();
                }
            );

        const auto database_config =
            timed_step(
                "Loading database configuration",
                [] {
                    return database::
                        load_config_from_environment();
                }
            );

        std::cout
            << "Scrape year: "
            << year
            << '\n'
            << "Database target: "
            << database_config.base_url
            << "\n\n";

        auto static_data =
            timed_step(
                "Loading and validating static room data",
                [] {
                    auto result =
                        data::load_static_data(
                            "data"
                        );

                    data::validate_static_data(
                        result
                    );

                    return result;
                }
            );

        std::cout
            << "  Buildings: "
            << static_data.buildings.size()
            << '\n'
            << "  Rooms: "
            << static_data.rooms.size()
            << "\n\n";

        http::Client http_client;

        publish::Client publish_client{
            http_client
        };

        const auto view_options =
            timed_step(
                "Fetching Publish view options",
                [&] {
                    return publish_client
                        .get_view_options();
                }
            );

        const auto location_ids =
            timed_step(
                "Collecting Publish room IDs",
                [&] {
                    std::vector<std::string>
                        result;

                    result.reserve(
                        static_data.rooms.size()
                    );

                    for (
                        const auto& room :
                        static_data.rooms
                    ) {
                        if (
                            !room.publish_id
                                .has_value()
                        ) {
                            continue;
                        }

                        result.push_back(
                            *room.publish_id
                        );
                    }

                    return result;
                }
            );

        std::cout
            << "  Rooms with Publish IDs: "
            << location_ids.size()
            << "\n\n";

        const auto events =
            timed_step(
                "Fetching Publish events",
                [&] {
                    return publish_client
                        .get_events(
                            location_ids,
                            view_options,
                            year
                        );
                }
            );

        const auto raw_event_count =
            count_event_rows(
                events
            );

        std::cout
            << "  Returned room categories: "
            << events.category_events.size()
            << '\n'
            << "  Raw room-event rows: "
            << raw_event_count
            << "\n\n";

        const auto image_count =
            timed_step(
                "Extracting Publish room images",
                [&] {
                    return rooms::
                        apply_publish_image_urls(
                            static_data.rooms,
                            events
                        );
                }
            );

        std::cout
            << "  Rooms with image URLs: "
            << image_count
            << "\n\n";

        auto bookings =
            timed_step(
                "Transforming Publish events into bookings",
                [&] {
                    return bookings::
                        transform_publish_events(
                            events,
                            static_data.rooms
                        );
                }
            );

        if (
            bookings.size()
            != raw_event_count
        ) {
            throw std::runtime_error{
                "Booking transformation changed "
                "the number of event rows"
            };
        }

        const auto removed_count =
            timed_step(
                "Filtering non-occupancy bookings",
                [&] {
                    return bookings::
                        filter_bookings_for_occupancy(
                            bookings
                        );
                }
            );

        std::cout
            << "  Removed: "
            << removed_count
            << '\n'
            << "  Remaining bookings: "
            << bookings.size()
            << "\n\n";

        auto building_payload =
            timed_step(
                "Serializing buildings",
                [&] {
                    return database::
                        serialize_buildings(
                            static_data.buildings,
                            static_data.rooms
                        );
                }
            );

        auto room_payload =
            timed_step(
                "Serializing rooms",
                [&] {
                    return database::
                        serialize_rooms(
                            static_data.rooms
                        );
                }
            );

        auto booking_payload =
            timed_step(
                "Serializing bookings",
                [&] {
                    return bookings::
                        serialize_bookings(
                            bookings
                        );
                }
            );

        auto module_payload =
            timed_step(
                "Serializing booking modules",
                [&] {
                    return bookings::
                        serialize_booking_modules(
                            bookings
                        );
                }
            );

        const auto building_count =
            building_payload.size();

        const auto room_count =
            room_payload.size();

        const auto booking_count =
            booking_payload.size();

        const auto module_count =
            module_payload.size();

        auto batch_request =
            timed_step(
                "Building Hasuragres transaction",
                [&] {
                    return database::
                        build_batch_request(
                            std::move(
                                building_payload
                            ),
                            std::move(
                                room_payload
                            ),
                            std::move(
                                booking_payload
                            ),
                            std::move(
                                module_payload
                            ),
                            year
                        );
                }
            );

        if (
            !batch_request.is_array()
            || batch_request.size() != 4
        ) {
            throw std::runtime_error{
                "Database transaction must "
                "contain four table inserts"
            };
        }

        std::cout
            << "Database payload:\n"
            << "  Buildings: "
            << building_count
            << '\n'
            << "  Rooms: "
            << room_count
            << '\n'
            << "  Bookings: "
            << booking_count
            << '\n'
            << "  BookingModules: "
            << module_count
            << "\n\n";

        database::Client database_client{
            http_client,
            database_config
        };

        const auto insert_result =
            timed_step(
                "Uploading database transaction",
                [&] {
                    return database_client
                        .batch_insert(
                            batch_request
                        );
                }
            );

        const auto request_mebibytes =
            static_cast<double>(
                insert_result.request_bytes
            )
            / 1024.0
            / 1024.0;

        std::cout
            << "Database insert complete:\n"
            << "  HTTP status: "
            << insert_result.status_code
            << '\n'
            << "  Request size: "
            << request_mebibytes
            << " MiB\n";

        if (
            !insert_result
                .response_body
                .empty()
        ) {
            std::cout
                << "  Response: "
                << insert_result.response_body
                << '\n';
        }

        std::cout
            << "\nScrape completed successfully in ";

        print_elapsed(
            elapsed_seconds(
                total_start
            )
        );

        std::cout
            << '\n';
    }
    catch (
        const std::exception& error
    ) {
        std::cerr
            << "\nScrape failed after ";

        print_elapsed(
            elapsed_seconds(
                total_start
            )
        );

        std::cerr
            << "\nError: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}