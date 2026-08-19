#include "config/exclusions.hpp"
#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "rooms/exclusions.hpp"
#include "rooms/publish_mapping.hpp"
#include "rooms/room_id.hpp"

#include <array>
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
            << "Publish API connection successful.\n\n"
            << "Time periods: "
            << view_options.time_periods.size()
            << '\n'
            << "Date periods: "
            << view_options.date_periods.size()
            << '\n'
            << "Weeks: "
            << view_options.weeks.size()
            << '\n'
            << "Days: "
            << view_options.days.size()
            << '\n';

        std::cout
            << "\nDate periods:\n";

        for (const auto& period : view_options.date_periods) {
            std::cout
                << "  "
                << period.description;

            if (period.is_default) {
                std::cout << " [default]";
            }

            std::cout << '\n';
        }

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
            << "\nRooms with Publish IDs: "
            << location_ids.size()
            << '\n';

        const auto events =
            publish_client.get_events(
                location_ids,
                view_options,
                2026
            );

        std::size_t total_event_rows = 0;

        const std::unordered_set<std::string>
            requested_location_ids{
                location_ids.begin(),
                location_ids.end()
            };

        std::unordered_set<std::string>
            returned_location_ids;

        std::unordered_set<std::string>
            unique_occurrence_ids;

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
                    "Publish returned an unrequested location: "
                    + category.identity
                };
            }

            const auto inserted =
                returned_location_ids
                    .insert(category.identity)
                    .second;

            if (!inserted) {
                throw std::runtime_error{
                    "Publish returned duplicate room category: "
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

        std::size_t deleted_events = 0;
        std::size_t unpublished_events = 0;
        std::size_t edited_events = 0;
        std::size_t manually_added_events = 0;
        std::size_t booking_events = 0;

        std::map<std::string, std::size_t>
            event_type_counts;

        std::map<std::string, std::size_t>
            source_counts;

        std::map<std::string, std::size_t>
            extra_property_counts;

        std::map<
            std::string,
            std::array<std::size_t, 2>
        > booking_flag_counts;

        std::unordered_set<std::string>
            room_occurrence_keys;

        std::size_t duplicate_room_occurrences = 0;
        std::size_t empty_occurrence_ids = 0;
        std::size_t empty_event_ids = 0;

        for (
            const auto& category :
            events.category_events
        ) {
            for (
                const auto& event :
                category.results
            ) {
                if (event.is_deleted) {
                    ++deleted_events;
                }

                if (!event.is_published) {
                    ++unpublished_events;
                }

                if (event.is_edited) {
                    ++edited_events;
                }

                if (
                    event.user_manually_added_event
                ) {
                    ++manually_added_events;
                }

                if (event.is_booking) {
                    ++booking_events;
                }

                ++event_type_counts[
                    event.event_type
                ];

                ++source_counts[
                    event.source.value_or("<null>")
                ];

                ++booking_flag_counts[
                    event.event_type
                ][event.is_booking ? 1 : 0];

                for (
                    const auto& property :
                    event.extra_properties
                ) {
                    ++extra_property_counts[
                        property.name
                    ];
                }

                if (event.identity.empty()) {
                    ++empty_occurrence_ids;
                }

                if (event.event_identity.empty()) {
                    ++empty_event_ids;
                }

                const auto room_occurrence_key =
                    category.identity
                    + ":"
                    + event.identity;

                if (
                    !room_occurrence_keys
                        .insert(
                            room_occurrence_key
                        )
                        .second
                ) {
                    ++duplicate_room_occurrences;
                }
            }
        }

        std::cout
            << "\nEvent status summary:\n"
            << "  IsBooking: "
            << booking_events
            << '\n'
            << "  Deleted: "
            << deleted_events
            << '\n'
            << "  Unpublished: "
            << unpublished_events
            << '\n'
            << "  Edited: "
            << edited_events
            << '\n'
            << "  Manually added: "
            << manually_added_events
            << '\n';

        std::cout
            << "\nEvent types:\n";

        for (
            const auto& [event_type, count] :
            event_type_counts
        ) {
            std::cout
                << "  "
                << event_type
                << ": "
                << count
                << '\n';
        }

        std::cout
            << "\nEvent sources:\n";

        for (
            const auto& [source, count] :
            source_counts
        ) {
            std::cout
                << "  "
                << source
                << ": "
                << count
                << '\n';
        }

        std::cout
            << "\nExtra properties:\n";

        for (
            const auto& [property, count] :
            extra_property_counts
        ) {
            std::cout
                << "  "
                << property
                << ": "
                << count
                << '\n';
        }

        std::cout
            << "\nBooking flag by relevant event type:\n";

        for (
            const auto& [event_type, counts] :
            booking_flag_counts
        ) {
            if (
                event_type != "*Booking"
                && !event_type.starts_with(
                    "BOOK."
                )
            ) {
                continue;
            }

            std::cout
                << "  "
                << event_type
                << ":\n"
                << "    IsBooking=false: "
                << counts[0]
                << '\n'
                << "    IsBooking=true: "
                << counts[1]
                << '\n';
        }

        std::cout
            << "\nBooking key validation:\n"
            << "  Duplicate (room, occurrence) pairs: "
            << duplicate_room_occurrences
            << '\n'
            << "  Empty occurrence IDs: "
            << empty_occurrence_ids
            << '\n'
            << "  Empty event IDs: "
            << empty_event_ids
            << '\n';

        const auto locations =
            publish_client.get_locations();

        std::cout
            << "\nAll Publish locations loaded.\n"
            << "Publish locations: "
            << locations.size()
            << '\n';

        const auto mapping =
            rooms::match_publish_locations(
                static_data.rooms,
                locations
            );

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

        if (!mapping.missing_from_publish.empty()) {
            std::cout
                << "\nStatic rooms missing from Publish:\n";

            for (
                const auto& room_id :
                mapping.missing_from_publish
            ) {
                std::cout
                    << "  "
                    << room_id
                    << '\n';
            }
        }

        if (
            !mapping
                .duplicate_publish_room_ids
                .empty()
        ) {
            std::cout
                << "\nDuplicate Publish room IDs:\n";

            for (
                const auto& room_id :
                mapping
                    .duplicate_publish_room_ids
            ) {
                std::cout
                    << "  "
                    << room_id
                    << '\n';
            }
        }

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