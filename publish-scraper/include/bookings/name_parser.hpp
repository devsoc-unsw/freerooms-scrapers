#pragma once

#include "types/booking.hpp"

#include <string>
#include <string_view>

namespace bookings {

enum class NamePattern {
    OWeek,
    Block,
    Exam,
    MiscClass,
    Class,
    InternalSociety,
    Society,
    Internal,
    Misc,
};

struct ParsedBookingName {
    model::BookingType booking_type;
    std::string name;
    NamePattern pattern;
};

ParsedBookingName parse_booking_name(std::string_view raw_name);

} // namespace bookings