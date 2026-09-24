#include "bookings/deduplicate.hpp"

#include <set>
#include <tuple>

namespace bookings {

std::size_t deduplicate_bookings_for_occupancy(std::vector<model::Booking>& bookings) {
    using OccupancyKey = std::tuple<std::string, std::string, std::string, std::string>;

    std::set<OccupancyKey> seen;

    const auto before = bookings.size();

    std::erase_if(bookings, [&](const model::Booking& booking) {
        const auto key = OccupancyKey{booking.room_id, booking.start, booking.end, booking.name};

        return !seen.insert(key).second;
    });

    return before - bookings.size();
}

} // namespace bookings
