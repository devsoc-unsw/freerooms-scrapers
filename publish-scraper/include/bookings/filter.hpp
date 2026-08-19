#pragma once

#include "types/booking.hpp"

#include <cstddef>
#include <vector>

namespace bookings {

bool should_include_booking(
    const model::Booking& booking
);

std::size_t filter_bookings_for_occupancy(
    std::vector<model::Booking>& bookings
);

}