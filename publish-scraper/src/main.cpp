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

namespace {

std::string booking_description(
    const model::Booking& booking
) {
    return booking.room_id
        + " | "
        + booking.event_type
        + " | "
        + booking.raw_name
        + " -> "
        + booking.name;
}

void add_example(
    std::vector<std::string>& examples,
    const model::Booking& booking,
    const std::size_t limit = 8
) {
    if (examples.size() >= limit) {
        return;
    }

    examples.push_back(
        booking_description(
            booking
        )
    );
}

} // namespace

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
        std::size_t empty_names = 0;
        std::size_t unknown_bookings = 0;

        std::map<std::string, std::size_t>
            booking_type_counts;

        std::map<std::string, std::size_t>
            event_type_counts;

        std::vector<std::string>
            society_examples;

        std::vector<std::string>
            block_examples;

        std::vector<std::string>
            oweek_examples;

        std::vector<std::string>
            requested_examples;

        std::vector<std::string>
            cancelled_examples;

        std::vector<std::string>
            not_used_examples;

        std::size_t society_rows = 0;
        std::size_t block_rows = 0;
        std::size_t oweek_rows = 0;

        std::size_t requested_rows = 0;
        std::size_t cancelled_rows = 0;
        std::size_t not_used_rows = 0;

        for (
            const auto& booking :
            bookings
        ) {
            const auto booking_key =
                booking.room_id
                + ":"
                + booking.occurrence_id;

            if (
                !booking_keys
                    .insert(
                        booking_key
                    )
                    .second
            ) {
                ++duplicate_booking_keys;
            }

            if (booking.name.empty()) {
                ++empty_names;
            }

            if (
                booking.booking_type
                == model::BookingType::Unknown
            ) {
                ++unknown_bookings;
            }

            const auto booking_type =
                std::string{
                    bookings::booking_type_name(
                        booking.booking_type
                    )
                };

            ++booking_type_counts[
                booking_type
            ];

            ++event_type_counts[
                booking.event_type
            ];

            if (
                booking.booking_type
                == model::BookingType::Society
            ) {
                ++society_rows;

                add_example(
                    society_examples,
                    booking
                );
            }

            if (
                booking.booking_type
                == model::BookingType::Block
            ) {
                ++block_rows;

                add_example(
                    block_examples,
                    booking
                );
            }

            if (
                booking.name.starts_with(
                    "OWeek"
                )
            ) {
                ++oweek_rows;

                add_example(
                    oweek_examples,
                    booking
                );
            }

            if (
                booking.event_type
                == "BOOK.REQUESTED"
            ) {
                ++requested_rows;

                add_example(
                    requested_examples,
                    booking
                );
            }

            if (
                booking.event_type
                == "BOOK.CANCELLED"
            ) {
                ++cancelled_rows;

                add_example(
                    cancelled_examples,
                    booking
                );
            }

            if (
                booking.event_type
                == "*Not Used"
            ) {
                ++not_used_rows;

                add_example(
                    not_used_examples,
                    booking
                );
            }
        }

        std::cout
            << "\nBooking transformation validation:\n"
            << "  Booking objects: "
            << bookings.size()
            << '\n'
            << "  Duplicate booking keys: "
            << duplicate_booking_keys
            << '\n'
            << "  Empty cleaned names: "
            << empty_names
            << '\n'
            << "  Unknown classifications: "
            << unknown_bookings
            << '\n';

        std::cout
            << "\nBooking classification:\n";

        for (
            const auto& [type, count] :
            booking_type_counts
        ) {
            std::cout
                << "  "
                << type
                << ": "
                << count
                << '\n';
        }

        std::cout
            << "\nClassification regression checks:\n"
            << "  Society rows: "
            << society_rows
            << '\n'
            << "  Block rows: "
            << block_rows
            << '\n'
            << "  OWeek rows: "
            << oweek_rows
            << '\n';

        if (!society_examples.empty()) {
            std::cout
                << "\nSociety examples:\n";

            for (
                const auto& example :
                society_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

        if (!block_examples.empty()) {
            std::cout
                << "\nBlock examples:\n";

            for (
                const auto& example :
                block_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

        if (!oweek_examples.empty()) {
            std::cout
                << "\nOWeek examples:\n";

            for (
                const auto& example :
                oweek_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

        std::cout
            << "\nPotential occupancy statuses:\n"
            << "  BOOK.REQUESTED: "
            << requested_rows
            << '\n'
            << "  BOOK.CANCELLED: "
            << cancelled_rows
            << '\n'
            << "  *Not Used: "
            << not_used_rows
            << '\n';

        if (!requested_examples.empty()) {
            std::cout
                << "\nBOOK.REQUESTED examples:\n";

            for (
                const auto& example :
                requested_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

        if (!cancelled_examples.empty()) {
            std::cout
                << "\nBOOK.CANCELLED examples:\n";

            for (
                const auto& example :
                cancelled_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

        if (!not_used_examples.empty()) {
            std::cout
                << "\n*Not Used examples:\n";

            for (
                const auto& example :
                not_used_examples
            ) {
                std::cout
                    << "  "
                    << example
                    << '\n';
            }
        }

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