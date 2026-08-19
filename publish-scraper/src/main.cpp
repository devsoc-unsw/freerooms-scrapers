#include "bookings/classification.hpp"
#include "bookings/filter.hpp"
#include "bookings/json.hpp"
#include "bookings/transform.hpp"
#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"

#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

int main() {
    try {
        const auto static_data =
            data::load_static_data("data");

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

        std::cout
            << "Publish API connection successful.\n";

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
            << "Rooms with Publish IDs: "
            << location_ids.size()
            << '\n';

        const auto events =
            publish_client.get_events(
                location_ids,
                view_options,
                2026
            );

        const std::unordered_set<std::string>
            requested_location_ids{
                location_ids.begin(),
                location_ids.end()
            };

        std::unordered_set<std::string>
            returned_location_ids;

        std::size_t total_event_rows = 0;

        for (
            const auto& category :
            events.category_events
        ) {
            if (
                !requested_location_ids.contains(
                    category.identity
                )
            ) {
                throw std::runtime_error{
                    "Publish returned unrequested location: "
                    + category.identity
                };
            }

            const auto inserted =
                returned_location_ids
                    .insert(
                        category.identity
                    )
                    .second;

            if (!inserted) {
                throw std::runtime_error{
                    "Publish returned duplicate location: "
                    + category.identity
                };
            }

            total_event_rows +=
                category.results.size();
        }

        std::size_t missing_location_count = 0;

        for (
            const auto& location_id :
            location_ids
        ) {
            if (
                !returned_location_ids.contains(
                    location_id
                )
            ) {
                ++missing_location_count;
            }
        }

        std::cout
            << "\nPublish event scrape complete.\n"
            << "  Requested room categories: "
            << location_ids.size()
            << '\n'
            << "  Returned room categories: "
            << events.category_events.size()
            << '\n'
            << "  Missing room categories: "
            << missing_location_count
            << '\n'
            << "  Raw room-event rows: "
            << total_event_rows
            << '\n';

        auto bookings =
            bookings::transform_publish_events(
                events,
                static_data.rooms
            );

        if (
            bookings.size()
            != total_event_rows
        ) {
            throw std::runtime_error{
                "Booking transformation changed "
                "the number of event rows"
            };
        }

        const auto removed_booking_count =
            bookings::filter_bookings_for_occupancy(
                bookings
            );

        std::cout
            << "\nOccupancy pipeline complete.\n"
            << "  Raw bookings: "
            << total_event_rows
            << '\n'
            << "  Removed: "
            << removed_booking_count
            << '\n'
            << "  Final bookings: "
            << bookings.size()
            << '\n';

        std::unordered_set<std::string>
            known_room_ids;

        for (
            const auto& room :
            static_data.rooms
        ) {
            known_room_ids.insert(
                room.id
            );
        }

        std::unordered_set<std::string>
            booking_keys;

        std::size_t duplicate_keys = 0;
        std::size_t unknown_rooms = 0;

        std::size_t empty_occurrence_ids = 0;
        std::size_t empty_event_ids = 0;

        std::size_t empty_names = 0;
        std::size_t empty_raw_names = 0;
        std::size_t empty_event_types = 0;

        std::size_t empty_start_times = 0;
        std::size_t empty_end_times = 0;

        std::size_t missing_last_modified = 0;

        std::size_t total_module_objects = 0;
        std::size_t modules_with_empty_codes = 0;
        std::size_t modules_with_empty_names = 0;

        std::map<std::string, std::size_t>
            database_type_counts;

        for (
            const auto& booking :
            bookings
        ) {
            const auto key =
                booking.room_id
                + ":"
                + booking.occurrence_id;

            if (
                !booking_keys
                    .insert(key)
                    .second
            ) {
                ++duplicate_keys;
            }

            if (
                !known_room_ids.contains(
                    booking.room_id
                )
            ) {
                ++unknown_rooms;
            }

            if (
                booking.occurrence_id.empty()
            ) {
                ++empty_occurrence_ids;
            }

            if (booking.event_id.empty()) {
                ++empty_event_ids;
            }

            if (booking.name.empty()) {
                ++empty_names;
            }

            if (booking.raw_name.empty()) {
                ++empty_raw_names;
            }

            if (booking.event_type.empty()) {
                ++empty_event_types;
            }

            if (booking.start.empty()) {
                ++empty_start_times;
            }

            if (booking.end.empty()) {
                ++empty_end_times;
            }

            if (
                !booking
                    .last_modified
                    .has_value()
            ) {
                ++missing_last_modified;
            }

            total_module_objects +=
                booking.modules.size();

            for (
                const auto& module :
                booking.modules
            ) {
                if (module.code.empty()) {
                    ++modules_with_empty_codes;
                }

                if (module.name.empty()) {
                    ++modules_with_empty_names;
                }
            }

            const auto database_type =
                std::string{
                    bookings::booking_type_database_value(
                        booking.booking_type
                    )
                };

            ++database_type_counts[
                database_type
            ];
        }

        std::cout
            << "\nDatabase field validation:\n"
            << "  Duplicate primary keys: "
            << duplicate_keys
            << '\n'
            << "  Unknown room IDs: "
            << unknown_rooms
            << '\n'
            << "  Empty occurrence IDs: "
            << empty_occurrence_ids
            << '\n'
            << "  Empty event IDs: "
            << empty_event_ids
            << '\n'
            << "  Empty names: "
            << empty_names
            << '\n'
            << "  Empty raw names: "
            << empty_raw_names
            << '\n'
            << "  Empty event types: "
            << empty_event_types
            << '\n'
            << "  Empty start times: "
            << empty_start_times
            << '\n'
            << "  Empty end times: "
            << empty_end_times
            << '\n'
            << "  Missing lastModified: "
            << missing_last_modified
            << '\n';

        std::cout
            << "\nModule model validation:\n"
            << "  Module objects: "
            << total_module_objects
            << '\n'
            << "  Modules with empty codes: "
            << modules_with_empty_codes
            << '\n'
            << "  Modules with empty names: "
            << modules_with_empty_names
            << '\n';

        std::cout
            << "\nDatabase booking types:\n";

        for (
            const auto& [type, count] :
            database_type_counts
        ) {
            std::cout
                << "  "
                << type
                << ": "
                << count
                << '\n';
        }

        if (
            duplicate_keys != 0
            || unknown_rooms != 0
            || empty_occurrence_ids != 0
            || empty_event_ids != 0
            || empty_names != 0
            || empty_raw_names != 0
            || empty_event_types != 0
            || empty_start_times != 0
            || empty_end_times != 0
            || modules_with_empty_codes != 0
            || modules_with_empty_names != 0
        ) {
            throw std::runtime_error{
                "Database model validation failed"
            };
        }

        const auto booking_payload =
            bookings::serialize_bookings(
                bookings
            );

        if (!booking_payload.is_array()) {
            throw std::runtime_error{
                "Serialized booking payload "
                "is not an array"
            };
        }

        if (
            booking_payload.size()
            != bookings.size()
        ) {
            throw std::runtime_error{
                "Serialized booking count does "
                "not match model booking count"
            };
        }

        if (booking_payload.empty()) {
            throw std::runtime_error{
                "Serialized booking payload "
                "is empty"
            };
        }

        const std::vector<std::string>
            required_booking_fields{
                "bookingType",
                "name",
                "roomId",
                "occurrenceId",
                "eventId",
                "rawName",
                "eventType",
                "start",
                "end",
                "plannedSize",
                "source",
                "lastModified",
            };

        for (
            const auto& field :
            required_booking_fields
        ) {
            if (
                !booking_payload
                    .front()
                    .contains(field)
            ) {
                throw std::runtime_error{
                    "Serialized booking is "
                    "missing field: "
                    + field
                };
            }
        }

        const auto module_payload =
            bookings::serialize_booking_modules(
                bookings
            );

        if (!module_payload.is_array()) {
            throw std::runtime_error{
                "Serialized module payload "
                "is not an array"
            };
        }

        if (
            module_payload.size()
            != total_module_objects
        ) {
            throw std::runtime_error{
                "Serialized module count does "
                "not match model module count"
            };
        }

        const std::vector<std::string>
            required_module_fields{
                "roomId",
                "occurrenceId",
                "moduleIndex",
                "code",
                "name",
                "term",
                "career",
            };

        if (!module_payload.empty()) {
            for (
                const auto& field :
                required_module_fields
            ) {
                if (
                    !module_payload
                        .front()
                        .contains(field)
                ) {
                    throw std::runtime_error{
                        "Serialized module is "
                        "missing field: "
                        + field
                    };
                }
            }
        }

        std::unordered_set<std::string>
            serialized_module_keys;

        std::size_t duplicate_module_keys = 0;
        std::size_t module_rows_for_unknown_bookings = 0;
        std::size_t serialized_modules_with_empty_codes = 0;
        std::size_t serialized_modules_with_empty_names = 0;

        for (
            const auto& module :
            module_payload
        ) {
            const auto room_id =
                module.at("roomId")
                    .get<std::string>();

            const auto occurrence_id =
                module.at("occurrenceId")
                    .get<std::string>();

            const auto module_index =
                module.at("moduleIndex")
                    .get<std::size_t>();

            const auto module_key =
                room_id
                + ":"
                + occurrence_id
                + ":"
                + std::to_string(
                    module_index
                );

            if (
                !serialized_module_keys
                    .insert(module_key)
                    .second
            ) {
                ++duplicate_module_keys;
            }

            const auto booking_key =
                room_id
                + ":"
                + occurrence_id;

            if (
                !booking_keys.contains(
                    booking_key
                )
            ) {
                ++module_rows_for_unknown_bookings;
            }

            if (
                module.at("code")
                    .get<std::string>()
                    .empty()
            ) {
                ++serialized_modules_with_empty_codes;
            }

            if (
                module.at("name")
                    .get<std::string>()
                    .empty()
            ) {
                ++serialized_modules_with_empty_names;
            }
        }

        std::cout
            << "\nBookingModules validation:\n"
            << "  Serialized module rows: "
            << module_payload.size()
            << '\n'
            << "  Duplicate module primary keys: "
            << duplicate_module_keys
            << '\n'
            << "  Modules with unknown booking: "
            << module_rows_for_unknown_bookings
            << '\n'
            << "  Empty module codes: "
            << serialized_modules_with_empty_codes
            << '\n'
            << "  Empty module names: "
            << serialized_modules_with_empty_names
            << '\n';

        if (
            duplicate_module_keys != 0
            || module_rows_for_unknown_bookings != 0
            || serialized_modules_with_empty_codes != 0
            || serialized_modules_with_empty_names != 0
        ) {
            throw std::runtime_error{
                "BookingModules validation failed"
            };
        }

        const auto serialized_bookings =
            booking_payload.dump();

        const auto serialized_modules =
            module_payload.dump();

        const auto booking_payload_mebibytes =
            static_cast<double>(
                serialized_bookings.size()
            )
            / 1024.0
            / 1024.0;

        const auto module_payload_mebibytes =
            static_cast<double>(
                serialized_modules.size()
            )
            / 1024.0
            / 1024.0;

        const auto total_payload_mebibytes =
            booking_payload_mebibytes
            + module_payload_mebibytes;

        std::cout
            << "\nDatabase serialization complete.\n"
            << "  Serialized booking rows: "
            << booking_payload.size()
            << '\n'
            << "  Serialized module rows: "
            << module_payload.size()
            << '\n'
            << "  Booking JSON size: "
            << booking_payload_mebibytes
            << " MiB\n"
            << "  Module JSON size: "
            << module_payload_mebibytes
            << " MiB\n"
            << "  Combined JSON size: "
            << total_payload_mebibytes
            << " MiB\n";

        std::cout
            << "\nFirst serialized booking:\n"
            << booking_payload
                .front()
                .dump(2)
            << '\n';

        if (!module_payload.empty()) {
            std::cout
                << "\nFirst serialized module:\n"
                << module_payload
                    .front()
                    .dump(2)
                << '\n';
        }

        std::cout
            << "\nStage 7A validation successful.\n";
    }
    catch (const std::exception& error) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}