#include "bookings/transform.hpp"

#include "bookings/classification.hpp"
#include "bookings/modules.hpp"
#include "bookings/name_parser.hpp"

#include <charconv>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

struct EventExtraProperties {
    std::optional<std::string> planned_size;
    std::optional<std::string> week_pattern;
    std::optional<std::string> module_name;
    std::optional<std::string> module_description;
};

EventExtraProperties extract_extra_properties(const publish::Event& event) {
    EventExtraProperties result;

    for (const auto& property : event.extra_properties) {
        if (property.name == "Activity.PlannedSize" && !result.planned_size.has_value()) {
            result.planned_size = property.value;
        } else if (property.name == "Activity.TeachingWeekPattern_PatternAsArray" &&
                   !result.week_pattern.has_value()) {
            result.week_pattern = property.value;
        } else if (property.name == "Module Name" && !result.module_name.has_value()) {
            result.module_name = property.value;
        } else if (property.name == "Module Description" &&
                   !result.module_description.has_value()) {
            result.module_description = property.value;
        }

        if (result.planned_size.has_value() && result.week_pattern.has_value() &&
            result.module_name.has_value() && result.module_description.has_value()) {
            break;
        }
    }

    return result;
}

std::optional<int> parse_optional_int(const std::optional<std::string>& value) {
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }

    int result = 0;

    const auto begin = value->data();

    const auto end = begin + value->size();

    const auto conversion = std::from_chars(begin, end, result);

    if (conversion.ec != std::errc{} || conversion.ptr != end) {
        return std::nullopt;
    }

    return result;
}

} // namespace

namespace bookings {

std::vector<model::Booking> transform_publish_events(const publish::EventsResponse& events,
                                                     const std::vector<model::Room>& rooms) {
    std::unordered_map<std::string, std::string> room_ids_by_publish_id;
    room_ids_by_publish_id.reserve(rooms.size());

    for (const auto& room : rooms) {
        if (!room.publish_id.has_value()) {
            continue;
        }

        const auto inserted = room_ids_by_publish_id.emplace(*room.publish_id, room.id).second;

        if (!inserted) {
            throw std::runtime_error{"Duplicate publishId in rooms.json: " + *room.publish_id};
        }
    }

    std::size_t total_events = 0;

    for (const auto& category : events.category_events) {
        total_events += category.results.size();
    }

    std::vector<model::Booking> bookings;

    bookings.reserve(total_events);

    for (const auto& category : events.category_events) {
        const auto room = room_ids_by_publish_id.find(category.identity);

        if (room == room_ids_by_publish_id.end()) {
            throw std::runtime_error{"No room found for Publish location: " + category.identity};
        }

        for (const auto& event : category.results) {
            const auto properties = extract_extra_properties(event);

            const auto planned_size = parse_optional_int(properties.planned_size);

            const auto modules =
                parse_modules(properties.module_name, properties.module_description);

            const auto parsed_name = parse_booking_name(event.name);

            const auto booking_type = classify_booking(event.event_type, parsed_name);

            bookings.push_back(model::Booking{
                .room_id = room->second,

                .occurrence_id = event.identity,

                .event_id = event.event_identity,

                .start = event.start_date_time,

                .end = event.end_date_time,

                .name = parsed_name.name,

                .raw_name = event.name,

                .booking_type = booking_type,

                .event_type = event.event_type,

                .modules = modules,

                .module_name_raw = properties.module_name,

                .module_description_raw = properties.module_description,

                .planned_size = planned_size,

                .week_labels = event.week_labels,

                .week_pattern = properties.week_pattern,

                .source = event.source,

                .last_modified = event.last_modified,

                .is_booking = event.is_booking,

                .is_published = event.is_published,

                .is_deleted = event.is_deleted,
            });
        }
    }

    return bookings;
}

} // namespace bookings