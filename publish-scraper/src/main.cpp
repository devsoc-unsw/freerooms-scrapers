#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"
#include "rooms/publish_mapping.hpp"
#include "data/room_metadata.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
    try {
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
            << "Publish API connection successful.\n\n";

        std::cout
            << "Time periods: "
            << view_options.time_periods.size()
            << '\n';

        std::cout
            << "Date periods: "
            << view_options.date_periods.size()
            << '\n';

        std::cout
            << "Weeks: "
            << view_options.weeks.size()
            << '\n';

        std::cout
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
            << "  New/unrecognised Publish rooms: "
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

        if (!mapping.missing_from_static.empty()) {
            std::cout
                << "\nPublish rooms missing from static data:\n";

            for (
                const auto& location :
                mapping.missing_from_static
            ) {
                std::cout
                    << "  "
                    << location.name
                    << " -> "
                    << location.identity
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

        if (
            !mapping
                .duplicate_publish_room_ids
                .empty()
        ) {
            throw std::runtime_error{
                "Refusing to update rooms.json because "
                "duplicate Publish room IDs were found"
            };
        }

        data::write_publish_ids(
            "data/rooms.json",
            mapping.matches
        );

        std::cout
            << "\nUpdated publishId for "
            << mapping.matches.size()
            << " rooms.\n";
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