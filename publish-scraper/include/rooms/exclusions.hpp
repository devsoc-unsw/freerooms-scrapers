#pragma once

#include "config/exclusions.hpp"
#include "types/publish.hpp"
#include "types/room.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace rooms {

enum class ExclusionReason {
    Building,
    Room,
    VirtualLocation,
    Usage,
    School,
};

std::optional<ExclusionReason> get_publish_location_exclusion(const std::string& room_id,
                                                              const config::Exclusions& exclusions);

std::optional<ExclusionReason> get_static_room_exclusion(const model::Room& room,
                                                         const config::Exclusions& exclusions);

std::size_t filter_excluded_static_rooms(std::vector<model::Room>& rooms,
                                         const config::Exclusions& exclusions);

std::size_t filter_excluded_publish_locations(std::vector<publish::Category>& locations,
                                              const config::Exclusions& exclusions,
                                              const std::vector<model::Room>& known_rooms);

} // namespace rooms
