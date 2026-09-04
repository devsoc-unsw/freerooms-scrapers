#include "bookings/sort.hpp"

#include <algorithm>

namespace bookings {

void sort_bookings(std::vector<model::Booking>& bookings) {
    std::ranges::sort(bookings, [](const model::Booking& left, const model::Booking& right) {
        if (left.start != right.start) {
            return left.start < right.start;
        }

        if (left.room_id != right.room_id) {
            return left.room_id < right.room_id;
        }

        return left.occurrence_id < right.occurrence_id;
    });
}

} // namespace bookings
