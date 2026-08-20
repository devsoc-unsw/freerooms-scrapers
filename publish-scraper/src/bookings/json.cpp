#include "bookings/json.hpp"

#include "bookings/classification.hpp"

#include <cstddef>
#include <string>

namespace bookings {

nlohmann::json serialize_booking(const model::Booking& booking) {
    auto result = nlohmann::json::object();

    result["bookingType"] = std::string{booking_type_database_value(booking.booking_type)};

    result["name"] = booking.name;

    result["roomId"] = booking.room_id;

    result["occurrenceId"] = booking.occurrence_id;

    result["eventId"] = booking.event_id;

    result["rawName"] = booking.raw_name;

    result["eventType"] = booking.event_type;

    result["start"] = booking.start;

    result["end"] = booking.end;

    if (booking.planned_size.has_value()) {
        result["plannedSize"] = *booking.planned_size;
    } else {
        result["plannedSize"] = nullptr;
    }

    if (booking.source.has_value()) {
        result["source"] = *booking.source;
    } else {
        result["source"] = nullptr;
    }

    if (booking.last_modified.has_value()) {
        result["lastModified"] = *booking.last_modified;
    } else {
        result["lastModified"] = nullptr;
    }

    return result;
}

nlohmann::json serialize_bookings(const std::vector<model::Booking>& bookings) {
    auto result = nlohmann::json::array();

    auto& array = result.get_ref<nlohmann::json::array_t&>();

    array.reserve(bookings.size());

    for (const auto& booking : bookings) {
        result.push_back(serialize_booking(booking));
    }

    return result;
}

nlohmann::json serialize_booking_modules(const std::vector<model::Booking>& bookings) {
    auto result = nlohmann::json::array();

    std::size_t total_modules = 0;

    for (const auto& booking : bookings) {
        total_modules += booking.modules.size();
    }

    auto& array = result.get_ref<nlohmann::json::array_t&>();

    array.reserve(total_modules);

    for (const auto& booking : bookings) {
        for (std::size_t module_index = 0; module_index < booking.modules.size(); ++module_index) {
            const auto& module = booking.modules[module_index];

            auto row = nlohmann::json::object();

            row["roomId"] = booking.room_id;

            row["occurrenceId"] = booking.occurrence_id;

            row["moduleIndex"] = module_index;

            row["code"] = module.code;

            row["name"] = module.name;

            if (module.term.has_value()) {
                row["term"] = *module.term;
            } else {
                row["term"] = nullptr;
            }

            if (module.career.has_value()) {
                row["career"] = *module.career;
            } else {
                row["career"] = nullptr;
            }

            result.push_back(std::move(row));
        }
    }

    return result;
}

} // namespace bookings