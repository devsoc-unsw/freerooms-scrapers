#include "bookings/transform.hpp"

#include "bookings/classification.hpp"
#include "bookings/modules.hpp"
#include "bookings/name_parser.hpp"

#include <charconv>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

std::optional<std::string>
find_extra_property(
    const publish::Event& event,
    const std::string& name
) {
    for (
        const auto& property :
        event.extra_properties
    ) {
        if (property.name == name) {
            return property.value;
        }
    }

    return std::nullopt;
}

std::optional<int>
parse_optional_int(
    const std::optional<std::string>& value
) {
    if (
        !value.has_value()
        || value->empty()
    ) {
        return std::nullopt;
    }

    int result = 0;

    const auto begin =
        value->data();

    const auto end =
        begin + value->size();

    const auto conversion =
        std::from_chars(
            begin,
            end,
            result
        );

    if (
        conversion.ec != std::errc{}
        || conversion.ptr != end
    ) {
        return std::nullopt;
    }

    return result;
}

}

namespace bookings {

std::vector<model::Booking>
transform_publish_events(
    const publish::EventsResponse& events,
    const std::vector<model::Room>& rooms
) {
    std::unordered_map<
        std::string,
        std::string
    > room_ids_by_publish_id;

    for (const auto& room : rooms) {
        if (!room.publish_id.has_value()) {
            continue;
        }

        const auto inserted =
            room_ids_by_publish_id
                .emplace(
                    *room.publish_id,
                    room.id
                )
                .second;

        if (!inserted) {
            throw std::runtime_error{
                "Duplicate publishId in rooms.json: "
                + *room.publish_id
            };
        }
    }

    std::size_t total_events = 0;

    for (
        const auto& category :
        events.category_events
    ) {
        total_events +=
            category.results.size();
    }

    std::vector<model::Booking> bookings;

    bookings.reserve(total_events);

    for (
        const auto& category :
        events.category_events
    ) {
        const auto room =
            room_ids_by_publish_id.find(
                category.identity
            );

        if (
            room
            == room_ids_by_publish_id.end()
        ) {
            throw std::runtime_error{
                "No room found for Publish location: "
                + category.identity
            };
        }

        for (
            const auto& event :
            category.results
        ) {
            const auto planned_size =
                parse_optional_int(
                    find_extra_property(
                        event,
                        "Activity.PlannedSize"
                    )
                );

            const auto week_pattern =
                find_extra_property(
                    event,
                    "Activity.TeachingWeekPattern_PatternAsArray"
                );

            const auto module_name_raw =
                find_extra_property(
                    event,
                    "Module Name"
                );

            const auto module_description_raw =
                find_extra_property(
                    event,
                    "Module Description"
                );

            const auto modules =
                parse_modules(
                    module_name_raw,
                    module_description_raw
                );

            const auto parsed_name =
                parse_booking_name(
                    event.name
                );

            const auto booking_type =
                classify_booking(
                    event.event_type,
                    parsed_name
                );

            bookings.push_back(
                model::Booking{
                    .room_id =
                        room->second,

                    .occurrence_id =
                        event.identity,

                    .event_id =
                        event.event_identity,

                    .start =
                        event.start_date_time,

                    .end =
                        event.end_date_time,

                    .name =
                        parsed_name.name,

                    .raw_name =
                        event.name,

                    .booking_type =
                        booking_type,

                    .event_type =
                        event.event_type,

                    .modules =
                        modules,

                    .module_name_raw =
                        module_name_raw,

                    .module_description_raw =
                        module_description_raw,

                    .planned_size =
                        planned_size,

                    .week_labels =
                        event.week_labels,

                    .week_pattern =
                        week_pattern,

                    .source =
                        event.source,

                    .last_modified =
                        event.last_modified,

                    .is_booking =
                        event.is_booking,

                    .is_published =
                        event.is_published,

                    .is_deleted =
                        event.is_deleted,
                }
            );
        }
    }

    return bookings;
}

}