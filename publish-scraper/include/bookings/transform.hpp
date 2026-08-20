#pragma once

#include "types/booking.hpp"
#include "types/publish.hpp"
#include "types/room.hpp"

#include <vector>

namespace bookings {

std::vector<model::Booking> transform_publish_events(const publish::EventsResponse& events,
                                                     const std::vector<model::Room>& rooms);

}