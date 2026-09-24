#pragma once

#include "types/booking.hpp"

#include <cstddef>
#include <vector>

namespace bookings {

std::size_t deduplicate_bookings_for_occupancy(std::vector<model::Booking>& bookings);

} // namespace bookings
