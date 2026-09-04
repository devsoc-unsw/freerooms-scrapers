#pragma once

#include "types/building.hpp"
#include "types/room.hpp"

#include <filesystem>
#include <vector>

namespace data {

struct StaticData {
    std::vector<model::Building> buildings;
    std::vector<model::Room> rooms;
};

StaticData load_static_data(const std::filesystem::path& data_directory);

void validate_static_data(const StaticData& static_data);

} // namespace data
