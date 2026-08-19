#include "config/exclusions.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using json = nlohmann::json;

std::unordered_set<std::string> load_string_set(
    const json& root,
    const std::string& key
) {
    const auto values =
        root.at(key).get<std::vector<std::string>>();

    return {
        values.begin(),
        values.end()
    };
}

}

namespace config {

Exclusions load_exclusions(
    const std::filesystem::path& path
) {
    std::ifstream input{path};

    if (!input.is_open()) {
        throw std::runtime_error{
            "Could not open exclusions file: "
            + path.string()
        };
    }

    try {
        json root;
        input >> root;

        return Exclusions{
            .building_ids =
                load_string_set(
                    root,
                    "buildingIds"
                ),

            .room_ids =
                load_string_set(
                    root,
                    "roomIds"
                ),

            .virtual_location_ids =
                load_string_set(
                    root,
                    "virtualLocationIds"
                ),

            .usages =
                load_string_set(
                    root,
                    "usages"
                ),

            .schools =
                load_string_set(
                    root,
                    "schools"
                ),
        };
    }
    catch (const json::exception& error) {
        throw std::runtime_error{
            "Invalid exclusions config: "
            + std::string{error.what()}
        };
    }
}

}