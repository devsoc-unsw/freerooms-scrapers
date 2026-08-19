#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

namespace config {

struct Exclusions {
    std::unordered_set<std::string> building_ids;
    std::unordered_set<std::string> room_ids;
    std::unordered_set<std::string> virtual_location_ids;

    std::unordered_set<std::string> usages;
    std::unordered_set<std::string> schools;
};

Exclusions load_exclusions(
    const std::filesystem::path& path
);

}