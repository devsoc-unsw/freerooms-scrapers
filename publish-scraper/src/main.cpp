#include "types/booking.hpp"
#include "types/building.hpp"
#include "types/publish.hpp"
#include "types/room.hpp"

#include <iostream>

int main() {
    const freerooms::Building building{};
    const freerooms::Room room{};
    const freerooms::Booking booking{};
    const freerooms::PublishEvent publish_event{};

    std::cout
        << "FreeRooms Publish scraper skeleton compiled successfully.\n";

    std::cout
        << "Initial types loaded: "
        << "Building, Room, Booking, PublishEvent.\n";

    static_cast<void>(building);
    static_cast<void>(room);
    static_cast<void>(booking);
    static_cast<void>(publish_event);

    return 0;
}
