#include "bookings/filter.hpp"

#include <vector>

namespace bookings {

bool should_include_booking(const model::Booking& booking) {
    if (!booking.is_published) {
        return false;
    }

    if (booking.is_deleted) {
        return false;
    }

    if (booking.event_type == "BOOK.CANCELLED") {
        return false;
    }

    if (booking.event_type == "BOOK.REQUESTED") {
        return false;
    }

    if (booking.event_type == "*Not Used") {
        return false;
    }

    return true;
}

std::size_t filter_bookings_for_occupancy(std::vector<model::Booking>& bookings) {
    const auto before = bookings.size();

    std::erase_if(bookings,
                  [](const model::Booking& booking) { return !should_include_booking(booking); });

    return before - bookings.size();
}

} // namespace bookings