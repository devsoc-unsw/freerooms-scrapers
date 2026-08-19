#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/transform.hpp"
#include "data/static_data.hpp"
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

        std::cout
            << "Static database serialization:\n"
            << "  Buildings: "
            << building_payload.size()
            << '\n'
            << "  Rooms: "
            << room_payload.size()
            << "\n\n";

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

        const std::vector<std::string>
            expected_tables{
                "Buildings",
                "Rooms",
                "Bookings",
                "BookingModules"
            };

        const std::vector<std::size_t>
            expected_counts{
                building_count,
                room_count,
                booking_count,
                module_count
            };

        for (
            std::size_t index = 0;
            index < expected_tables.size();
            ++index
        ) {
            const auto& request =
                batch_request.at(index);

            const auto table_name =
                request
                    .at("metadata")
                    .at("table_name")
                    .get<std::string>();

            if (
                table_name
                != expected_tables[index]
            ) {
                throw std::runtime_error{
                    "Unexpected table at batch index "
                    + std::to_string(index)
                    + ": "
                    + table_name
                };
            }

            const auto payload_count =
                request
                    .at("payload")
                    .size();

            if (
                payload_count
                != expected_counts[index]
            ) {
                throw std::runtime_error{
                    "Payload count changed for table "
                    + table_name
                };
            }
        }

        if (
            batch_request
                .at(0)
                .at("metadata")
                .at("write_mode")
            != "overwrite"
        ) {
            throw std::runtime_error{
                "Buildings should use overwrite"
            };
        }

        if (
            batch_request
                .at(1)
                .at("metadata")
                .at("write_mode")
            != "overwrite"
        ) {
            throw std::runtime_error{
                "Rooms should use overwrite"
            };
        }

        if (
            batch_request
                .at(2)
                .at("metadata")
                .at("write_mode")
            != "append"
        ) {
            throw std::runtime_error{
                "Bookings should use append"
            };
        }

        if (
            batch_request
                .at(3)
                .at("metadata")
                .at("write_mode")
            != "append"
        ) {
            throw std::runtime_error{
                "BookingModules should use append"
            };
        }

        const auto& bookings_before =
            batch_request
                .at(2)
                .at("metadata")
                .at("sql_before")
                .get_ref<
                    const std::string&
                >();

        if (
            bookings_before.find("{0}")
                != std::string::npos
            || bookings_before.find("{1}")
                != std::string::npos
        ) {
            throw std::runtime_error{
                "Bookings sql_before still "
                "contains placeholders"
            };
        }

        std::cout
            << "\nFinal batch request:\n"
            << "  1. Buildings: "
            << batch_request
                .at(0)
                .at("payload")
                .size()
            << '\n'
            << "  2. Rooms: "
            << batch_request
                .at(1)
                .at("payload")
                .size()
            << '\n'
            << "  3. Bookings: "
            << batch_request
                .at(2)
                .at("payload")
                .size()
            << '\n'
            << "  4. BookingModules: "
            << batch_request
                .at(3)
                .at("payload")
                .size()
            << '\n';

        const auto serialized =
            batch_request.dump();

        const auto request_mebibytes =
            static_cast<double>(
                serialized.size()
            )
            / 1024.0
            / 1024.0;

        std::cout
            << "\nComplete /batch_insert body:\n"
            << "  Size: "
            << request_mebibytes
            << " MiB\n";

        std::cout
            << "\nBatch order:\n";

        for (
            std::size_t index = 0;
            index < batch_request.size();
            ++index
        ) {
            std::cout
                << "  "
                << index + 1
                << ". "
                << batch_request
                    .at(index)
                    .at("metadata")
                    .at("table_name")
                    .get<std::string>()
                << '\n';
        }

        std::cout
            << "\nStage 7B2 validation successful.\n";
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