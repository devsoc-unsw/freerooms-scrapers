#include "config/exclusions.hpp"
#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "rooms/exclusions.hpp"
#include "rooms/publish_mapping.hpp"
#include "rooms/room_id.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
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

        std::vector<std::string> test_location_ids;

        for (const auto& room : static_data.rooms) {
            if (!room.publish_id.has_value()) {
                continue;
            }

            test_location_ids.push_back(
                *room.publish_id
            );

            if (test_location_ids.size() == 2) {
                break;
            }
        }

        if (test_location_ids.size() != 2) {
            throw std::runtime_error{
                "Could not find two rooms with publishId"
            };
        }

        const auto events =
            publish_client.get_events(
                test_location_ids,
                view_options,
                2026
            );

        std::cout
            << "\nPublish event test successful.\n"
            << "Returned room categories: "
            << events.category_events.size()
            << '\n';

        for (
            const auto& category :
            events.category_events
        ) {
            std::cout
                << "\n"
                << category.name
                << '\n'
                << "  Publish ID: "
                << category.identity
                << '\n'
                << "  Events: "
                << category.results.size()
                << '\n';

            if (category.results.empty()) {
                continue;
            }

            const auto& event =
                category.results.front();

            std::cout
                << "  First event:\n"
                << "    Name: "
                << event.name
                << '\n'
                << "    Event type: "
                << event.event_type
                << '\n'
                << "    Start: "
                << event.start_date_time
                << '\n'
                << "    End: "
                << event.end_date_time
                << '\n'
                << "    EventIdentity: "
                << event.event_identity
                << '\n'
                << "    Identity: "
                << event.identity
                << '\n'
                << "    Extra properties: "
                << event.extra_properties.size()
                << '\n';
        }

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