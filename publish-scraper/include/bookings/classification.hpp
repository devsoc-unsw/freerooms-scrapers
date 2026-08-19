#pragma once

#include "types/booking.hpp"

#include <string_view>

namespace bookings {

model::BookingType classify_booking(
    std::string_view event_type
);

std::string_view booking_type_name(
    model::BookingType booking_type
);

}