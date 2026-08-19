#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/transform.hpp"
#include "data/static_data.hpp"
#include "database/request.hpp"
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

        const auto booking_count =
            booking_payload.size();

        const auto module_count =
            module_payload.size();

        std::cout
            << "\nSerialization complete.\n"
            << "  Booking rows: "
            << booking_count
            << '\n'
            << "  Module rows: "
            << module_count
            << '\n';

        auto batch_request =
            database::build_batch_request(
                std::move(
                    booking_payload
                ),
                std::move(
                    module_payload
                ),
                year
            );

        if (!batch_request.is_array()) {
            throw std::runtime_error{
                "Batch request is not an array"
            };
        }

        if (batch_request.size() != 2) {
            throw std::runtime_error{
                "Batch request should contain "
                "exactly two table inserts"
            };
        }

        const auto& bookings_request =
            batch_request.at(0);

        const auto& modules_request =
            batch_request.at(1);

        if (
            bookings_request
                .at("metadata")
                .at("table_name")
                != "Bookings"
        ) {
            throw std::runtime_error{
                "First batch table is not Bookings"
            };
        }

        if (
            modules_request
                .at("metadata")
                .at("table_name")
                != "BookingModules"
        ) {
            throw std::runtime_error{
                "Second batch table is not BookingModules"
            };
        }

        if (
            bookings_request
                .at("metadata")
                .at("write_mode")
                != "append"
        ) {
            throw std::runtime_error{
                "Bookings write mode is not append"
            };
        }

        if (
            modules_request
                .at("metadata")
                .at("write_mode")
                != "append"
        ) {
            throw std::runtime_error{
                "BookingModules write mode "
                "is not append"
            };
        }

        if (
            !bookings_request
                .at("metadata")
                .contains("sql_before")
        ) {
            throw std::runtime_error{
                "Bookings request has no sql_before"
            };
        }

        if (
            modules_request
                .at("metadata")
                .contains("sql_before")
        ) {
            throw std::runtime_error{
                "BookingModules should not "
                "have sql_before"
            };
        }

        if (
            bookings_request
                .at("payload")
                .size()
            != booking_count
        ) {
            throw std::runtime_error{
                "Bookings were lost while "
                "building batch request"
            };
        }

        if (
            modules_request
                .at("payload")
                .size()
            != module_count
        ) {
            throw std::runtime_error{
                "Modules were lost while "
                "building batch request"
            };
        }

        const auto& cleanup_sql =
            bookings_request
                .at("metadata")
                .at("sql_before")
                .get_ref<
                    const std::string&
                >();

        if (
            cleanup_sql.find("{0}")
                != std::string::npos
            || cleanup_sql.find("{1}")
                != std::string::npos
        ) {
            throw std::runtime_error{
                "Year placeholders remain "
                "in Bookings sql_before"
            };
        }

        if (
            cleanup_sql.find("2026")
                == std::string::npos
            || cleanup_sql.find("2027")
                == std::string::npos
        ) {
            throw std::runtime_error{
                "Bookings sql_before does "
                "not contain expected years"
            };
        }

        std::cout
            << "\nBatch request validation:\n"
            << "  Tables: "
            << batch_request.size()
            << '\n'
            << "  First table: "
            << bookings_request
                .at("metadata")
                .at("table_name")
                .get<std::string>()
            << '\n'
            << "  Second table: "
            << modules_request
                .at("metadata")
                .at("table_name")
                .get<std::string>()
            << '\n'
            << "  Booking rows: "
            << bookings_request
                .at("payload")
                .size()
            << '\n'
            << "  Module rows: "
            << modules_request
                .at("payload")
                .size()
            << '\n';

        std::cout
            << "\nBookings sql_before:\n"
            << cleanup_sql
            << '\n';

        const auto serialized_request =
            batch_request.dump();

        const auto request_mebibytes =
            static_cast<double>(
                serialized_request.size()
            )
            / 1024.0
            / 1024.0;

        std::cout
            << "\nComplete /batch_insert body:\n"
            << "  Size: "
            << request_mebibytes
            << " MiB\n";

        std::cout
            << "\nStage 7B1 validation successful.\n";
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