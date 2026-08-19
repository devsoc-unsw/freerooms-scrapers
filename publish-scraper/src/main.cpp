#include "bookings/classification.hpp"
#include "bookings/transform.hpp"
#include "config/exclusions.hpp"
#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "rooms/exclusions.hpp"
#include "rooms/publish_mapping.hpp"
#include "rooms/room_id.hpp"

#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

int main() {
    try {
        const auto exclusions =
            config::load_exclusions(
                "config/exclusions.json"
            );

        std::cout
            << "Exclusions loaded.\n"
            << "  Buildings: "
            << exclusions.building_ids.size()
            << '\n'
            << "  Rooms: "
            << exclusions.room_ids.size()
            << '\n'
            << "  Virtual locations: "
            << exclusions.virtual_location_ids.size()
            << '\n'
            << "  Usages: "
            << exclusions.usages.size()
            << '\n'
            << "  Schools: "
            << exclusions.schools.size()
            << "\n\n";

        const auto static_data =
            data::load_static_data("data");

        data::validate_static_data(static_data);

        std::cout
            << "Static data loaded successfully.\n"
            << "Buildings: "
            << static_data.buildings.size()
            << '\n'
            << "Rooms: "
            << static_data.rooms.size()
            << "\n\n";

        http::Client http_client;
        publish::Client publish_client{http_client};

        const auto view_options =
            publish_client.get_view_options();

        std::cout
            << "Publish API connection successful.\n";

        std::vector<std::string> location_ids;

        for (const auto& room : static_data.rooms) {
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

        std::unordered_set<std::string>
            unique_occurrence_ids;

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

            for (
                const auto& event :
                category.results
            ) {
                unique_occurrence_ids.insert(
                    event.identity
                );
            }
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
            << "  Room-event rows: "
            << total_event_rows
            << '\n'
            << "  Unique occurrence IDs: "
            << unique_occurrence_ids.size()
            << '\n';

        const auto bookings =
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

        std::unordered_set<std::string>
            booking_keys;

        std::size_t duplicate_booking_keys = 0;
        std::size_t missing_planned_size = 0;

        std::size_t bookings_with_raw_modules = 0;
        std::size_t bookings_with_parsed_modules = 0;
        std::size_t module_parse_failures = 0;
        std::size_t parsed_module_objects = 0;
        std::size_t modules_without_names = 0;

        std::size_t cancelled_rows = 0;
        std::size_t requested_rows = 0;
        std::size_t confirmed_rows = 0;

        std::map<std::string, std::size_t>
            booking_type_counts;

        std::map<std::string, std::size_t>
            unknown_event_types;

        for (const auto& booking : bookings) {
            const auto key =
                booking.room_id
                + ":"
                + booking.occurrence_id;

            if (
                !booking_keys
                    .insert(key)
                    .second
            ) {
                ++duplicate_booking_keys;
            }

            if (
                !booking
                    .planned_size
                    .has_value()
            ) {
                ++missing_planned_size;
            }

            const auto has_raw_modules =
                booking.module_name_raw.has_value();

            if (has_raw_modules) {
                ++bookings_with_raw_modules;
            }

            if (!booking.modules.empty()) {
                ++bookings_with_parsed_modules;
            }

            if (
                has_raw_modules
                && booking.modules.empty()
            ) {
                ++module_parse_failures;
            }

            parsed_module_objects +=
                booking.modules.size();

            for (
                const auto& module :
                booking.modules
            ) {
                if (module.name.empty()) {
                    ++modules_without_names;
                }
            }

            const auto booking_type =
                bookings::booking_type_name(
                    booking.booking_type
                );

            ++booking_type_counts[
                std::string{booking_type}
            ];

            if (
                booking.booking_type
                == model::BookingType::Unknown
            ) {
                ++unknown_event_types[
                    booking.event_type
                ];
            }

            if (
                booking.event_type
                == "BOOK.CANCELLED"
            ) {
                ++cancelled_rows;
            }

            if (
                booking.event_type
                == "BOOK.REQUESTED"
            ) {
                ++requested_rows;
            }

            if (
                booking.event_type
                == "BOOK.CONFIRMED"
            ) {
                ++confirmed_rows;
            }
        }

        std::cout
            << "\nBooking transformation complete.\n"
            << "  Raw room-event rows: "
            << total_event_rows
            << '\n'
            << "  Booking objects: "
            << bookings.size()
            << '\n'
            << "  Duplicate booking keys: "
            << duplicate_booking_keys
            << '\n'
            << "  Missing planned size: "
            << missing_planned_size
            << '\n';

        std::cout
            << "\nModule parsing:\n"
            << "  Bookings with raw module data: "
            << bookings_with_raw_modules
            << '\n'
            << "  Bookings with parsed modules: "
            << bookings_with_parsed_modules
            << '\n'
            << "  Module parse failures: "
            << module_parse_failures
            << '\n'
            << "  Parsed Module objects: "
            << parsed_module_objects
            << '\n'
            << "  Modules without names: "
            << modules_without_names
            << '\n';

        std::cout
            << "\nBooking classification:\n";

        for (
            const auto& [booking_type, count] :
            booking_type_counts
        ) {
            std::cout
                << "  "
                << booking_type
                << ": "
                << count
                << '\n';
        }

        if (!unknown_event_types.empty()) {
            std::cout
                << "\nUnknown event types:\n";

            for (
                const auto& [event_type, count] :
                unknown_event_types
            ) {
                std::cout
                    << "  "
                    << event_type
                    << ": "
                    << count
                    << '\n';
            }
        }

        std::cout
            << "\nRoom-booking statuses:\n"
            << "  BOOK.CONFIRMED: "
            << confirmed_rows
            << '\n'
            << "  BOOK.REQUESTED: "
            << requested_rows
            << '\n'
            << "  BOOK.CANCELLED: "
            << cancelled_rows
            << '\n';

        const auto locations =
            publish_client.get_locations();

        const auto mapping =
            rooms::match_publish_locations(
                static_data.rooms,
                locations
            );

        std::size_t excluded_by_building = 0;
        std::size_t excluded_by_room = 0;
        std::size_t excluded_virtual = 0;

        std::vector<publish::Category>
            candidate_new_rooms;

        for (
            const auto& location :
            mapping.missing_from_static
        ) {
            const auto room_id =
                rooms::extract_room_id(
                    location.name
                );

            if (!room_id.has_value()) {
                continue;
            }

            const auto exclusion =
                rooms::get_publish_location_exclusion(
                    *room_id,
                    exclusions
                );

            if (!exclusion.has_value()) {
                candidate_new_rooms.push_back(
                    location
                );

                continue;
            }

            switch (*exclusion) {
                case rooms::ExclusionReason::Building:
                    ++excluded_by_building;
                    break;

                case rooms::ExclusionReason::Room:
                    ++excluded_by_room;
                    break;

                case rooms::ExclusionReason::VirtualLocation:
                    ++excluded_virtual;
                    break;
            }
        }

        std::cout
            << "\nRoom mapping results:\n"
            << "  Static rooms: "
            << static_data.rooms.size()
            << '\n'
            << "  Matched: "
            << mapping.matches.size()
            << '\n'
            << "  Missing from Publish: "
            << mapping.missing_from_publish.size()
            << '\n'
            << "  Publish-only locations: "
            << mapping.missing_from_static.size()
            << '\n'
            << "  Duplicate Publish room IDs: "
            << mapping
                .duplicate_publish_room_ids
                .size()
            << '\n';

        std::cout
            << "\nPublish-only location classification:\n"
            << "  Total: "
            << mapping.missing_from_static.size()
            << '\n'
            << "  Excluded by building: "
            << excluded_by_building
            << '\n'
            << "  Excluded by room: "
            << excluded_by_room
            << '\n'
            << "  Virtual locations: "
            << excluded_virtual
            << '\n'
            << "  Candidate new rooms: "
            << candidate_new_rooms.size()
            << '\n';

        if (!candidate_new_rooms.empty()) {
            std::cout
                << "\nPotential new rooms to investigate:\n";

            for (
                const auto& location :
                candidate_new_rooms
            ) {
                std::cout
                    << "  "
                    << location.name
                    << " -> "
                    << location.identity
                    << '\n';
            }
        }
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