#include "bookings/classification.hpp"
#include "bookings/filter.hpp"
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

        std::unordered_set<std::string>
            raw_booking_keys;

        std::size_t duplicate_raw_keys = 0;
        std::size_t unpublished_rows = 0;
        std::size_t deleted_rows = 0;
        std::size_t requested_rows = 0;
        std::size_t cancelled_rows = 0;
        std::size_t not_used_rows = 0;

        for (
            const auto& booking :
            bookings
        ) {
            const auto key =
                booking.room_id
                + ":"
                + booking.occurrence_id;

            if (
                !raw_booking_keys
                    .insert(key)
                    .second
            ) {
                ++duplicate_raw_keys;
            }

            if (!booking.is_published) {
                ++unpublished_rows;
            }

            if (booking.is_deleted) {
                ++deleted_rows;
            }

            if (
                booking.event_type
                == "BOOK.REQUESTED"
            ) {
                ++requested_rows;
            }

            if (
                booking.event_type
                == "BOOK.CANCELLED"
            ) {
                ++cancelled_rows;
            }

            if (
                booking.event_type
                == "*Not Used"
            ) {
                ++not_used_rows;
            }
        }

        std::cout
            << "\nRaw booking transformation:\n"
            << "  Booking objects: "
            << bookings.size()
            << '\n'
            << "  Duplicate booking keys: "
            << duplicate_raw_keys
            << '\n';

        std::cout
            << "\nOccupancy exclusions before filtering:\n"
            << "  Unpublished: "
            << unpublished_rows
            << '\n'
            << "  Deleted: "
            << deleted_rows
            << '\n'
            << "  BOOK.REQUESTED: "
            << requested_rows
            << '\n'
            << "  BOOK.CANCELLED: "
            << cancelled_rows
            << '\n'
            << "  *Not Used: "
            << not_used_rows
            << '\n';

        const auto raw_booking_count =
            bookings.size();

        const auto removed_booking_count =
            bookings::filter_bookings_for_occupancy(
                bookings
            );

        std::unordered_set<std::string>
            occupancy_booking_keys;

        std::size_t duplicate_occupancy_keys = 0;
        std::size_t invalid_remaining_rows = 0;

        std::map<std::string, std::size_t>
            booking_type_counts;

        std::map<std::string, std::size_t>
            remaining_status_counts;

        for (
            const auto& booking :
            bookings
        ) {
            const auto key =
                booking.room_id
                + ":"
                + booking.occurrence_id;

            if (
                !occupancy_booking_keys
                    .insert(key)
                    .second
            ) {
                ++duplicate_occupancy_keys;
            }

            if (
                !bookings::should_include_booking(
                    booking
                )
            ) {
                ++invalid_remaining_rows;
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

            ++remaining_status_counts[
                booking.event_type
            ];
        }

        std::cout
            << "\nOccupancy filtering complete.\n"
            << "  Before filtering: "
            << raw_booking_count
            << '\n'
            << "  Removed: "
            << removed_booking_count
            << '\n'
            << "  Remaining bookings: "
            << bookings.size()
            << '\n'
            << "  Duplicate booking keys: "
            << duplicate_occupancy_keys
            << '\n'
            << "  Invalid rows remaining: "
            << invalid_remaining_rows
            << '\n';

        std::cout
            << "\nFinal booking classification:\n";

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
            << "\nExcluded statuses remaining:\n"
            << "  BOOK.REQUESTED: "
            << remaining_status_counts[
                "BOOK.REQUESTED"
            ]
            << '\n'
            << "  BOOK.CANCELLED: "
            << remaining_status_counts[
                "BOOK.CANCELLED"
            ]
            << '\n'
            << "  *Not Used: "
            << remaining_status_counts[
                "*Not Used"
            ]
            << '\n';

        if (invalid_remaining_rows != 0) {
            throw std::runtime_error{
                "Occupancy filter left invalid bookings"
            };
        }

        if (duplicate_occupancy_keys != 0) {
            throw std::runtime_error{
                "Duplicate booking keys remain after filtering"
            };
        }

        std::cout
            << "\nBooking pipeline validation successful.\n";
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