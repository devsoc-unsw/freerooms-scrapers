#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/transform.hpp"
#include "data/static_data.hpp"
#include "database/client.hpp"
#include "database/request.hpp"
#include "database/static_json.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

bool is_local_hasuragres(
    const std::string& base_url
) {
    return (
        base_url
        == "http://localhost:8000"
    )
    || (
        base_url
        == "http://127.0.0.1:8000"
    );
}

}

int main() {
    try {
        constexpr int year = 2026;

        const auto static_data =
            data::load_static_data(
                "data"
            );

        data::validate_static_data(
            static_data
        );

        std::cout
            << "Static data loaded successfully.\n"
            << "Buildings: "
            << static_data.buildings.size()
            << '\n'
            << "Rooms: "
            << static_data.rooms.size()
            << "\n\n";

        auto building_payload =
            database::serialize_buildings(
                static_data.buildings,
                static_data.rooms
            );

        auto room_payload =
            database::serialize_rooms(
                static_data.rooms
            );

        http::Client http_client;

        publish::Client publish_client{
            http_client
        };

        const auto view_options =
            publish_client.get_view_options();

        std::vector<std::string>
            location_ids;

        for (
            const auto& room :
            static_data.rooms
        ) {
            if (!room.publish_id.has_value()) {
                continue;
            }

            location_ids.push_back(
                *room.publish_id
            );
        }

        std::cout
            << "Publish API connection successful.\n"
            << "Rooms with Publish IDs: "
            << location_ids.size()
            << '\n';

        const auto events =
            publish_client.get_events(
                location_ids,
                view_options,
                year
            );

        std::size_t raw_event_rows = 0;

        for (
            const auto& category :
            events.category_events
        ) {
            raw_event_rows +=
                category.results.size();
        }

        auto bookings =
            bookings::transform_publish_events(
                events,
                static_data.rooms
            );

        if (
            bookings.size()
            != raw_event_rows
        ) {
            throw std::runtime_error{
                "Booking transformation changed "
                "the number of event rows"
            };
        }

        const auto removed =
            bookings::filter_bookings_for_occupancy(
                bookings
            );

        std::cout
            << "\nBooking pipeline complete.\n"
            << "  Raw events: "
            << raw_event_rows
            << '\n'
            << "  Removed: "
            << removed
            << '\n'
            << "  Final bookings: "
            << bookings.size()
            << '\n';

        auto booking_payload =
            bookings::serialize_bookings(
                bookings
            );

        auto module_payload =
            bookings::serialize_booking_modules(
                bookings
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
            database::build_batch_request(
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

        if (
            !batch_request.is_array()
            || batch_request.size() != 4
        ) {
            throw std::runtime_error{
                "Batch request must contain "
                "four table inserts"
            };
        }

        std::cout
            << "\nPrepared local database transaction:\n"
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
            << '\n';

        const auto database_config =
            database::load_config_from_environment();

        std::cout
            << "\nHasuragres target:\n"
            << "  "
            << database_config.base_url
            << '\n';

        if (
            !is_local_hasuragres(
                database_config.base_url
            )
        ) {
            throw std::runtime_error{
                "Stage 7C local test refuses "
                "to write to non-local Hasuragres"
            };
        }

        database::Client database_client{
            http_client,
            database_config
        };

        std::cout
            << "\nSending local /batch_insert...\n";

        const auto insert_result =
            database_client.batch_insert(
                batch_request
            );

        const auto request_mebibytes =
            static_cast<double>(
                insert_result.request_bytes
            )
            / 1024.0
            / 1024.0;

        std::cout
            << "\nLocal Hasuragres insert successful.\n"
            << "  HTTP status: "
            << insert_result.status_code
            << '\n'
            << "  Request size: "
            << request_mebibytes
            << " MiB\n"
            << "  Response: "
            << insert_result.response_body
            << '\n';

        std::cout
            << "\nStage 7C2 database write successful.\n";
    }
    catch (
        const std::exception& error
    ) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}