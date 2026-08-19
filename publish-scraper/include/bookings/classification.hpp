#pragma once

#include "bookings/name_parser.hpp"
#include "types/booking.hpp"

#include <string_view>

namespace bookings {

model::BookingType classify_booking(
    std::string_view event_type
);

model::BookingType classify_booking(
    std::string_view event_type,
    const ParsedBookingName& parsed_name
);

std::string_view booking_type_name(
    model::BookingType booking_type
);

std::string_view booking_type_database_value(
    model::BookingType booking_type
);

}