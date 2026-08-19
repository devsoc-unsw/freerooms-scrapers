#pragma once

#include "types/building.hpp"
#include "types/room.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace database {

nlohmann::json serialize_buildings(
    const std::vector<model::Building>& buildings,
    const std::vector<model::Room>& rooms
);

nlohmann::json serialize_rooms(
    const std::vector<model::Room>& rooms
);

}