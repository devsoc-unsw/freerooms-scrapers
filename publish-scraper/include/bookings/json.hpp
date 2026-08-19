#pragma once

#include "types/booking.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace bookings {

nlohmann::json serialize_booking(
    const model::Booking& booking
);

nlohmann::json serialize_bookings(
    const std::vector<model::Booking>& bookings
);

nlohmann::json serialize_booking_modules(
    const std::vector<model::Booking>& bookings
);

}