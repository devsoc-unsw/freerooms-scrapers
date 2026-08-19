#include "data/static_data.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

using json = nlohmann::json;

json read_json_file(const std::filesystem::path& path) {
    std::ifstream input{path};

    if (!input.is_open()) {
        throw std::runtime_error{
            "Could not open JSON file: " + path.string()
        };
    }

    try {
        json value;
        input >> value;
        return value;
    }
    catch (const json::exception& error) {
        throw std::runtime_error{
            "Invalid JSON in " + path.string() + ": " + error.what()
        };
    }
}

std::optional<model::FloorType> parse_floor(
    const json& value
) {
    if (value.is_null()) {
        return std::nullopt;
    }

    const auto floor = value.get<std::string>();

    if (floor == "Flat") {
        return model::FloorType::Flat;
    }

    if (floor == "Tiered") {
        return model::FloorType::Tiered;
    }

    if (floor == "Other") {
        return model::FloorType::Other;
    }

    throw std::runtime_error{
        "Unknown floor type: " + floor
    };
}

std::optional<model::SeatingType> parse_seating(
    const json& value
) {
    if (value.is_null()) {
        return std::nullopt;
    }

    const auto seating = value.get<std::string>();

    if (seating == "Movable") {
        return model::SeatingType::Movable;
    }

    if (seating == "Fixed") {
        return model::SeatingType::Fixed;
    }

    throw std::runtime_error{
        "Unknown seating type: " + seating
    };
}

model::RoomFacilities parse_facilities(
    const json& value
) {
    model::RoomFacilities facilities;

    facilities.floor = parse_floor(
        value.at("floor")
    );

    facilities.seating = parse_seating(
        value.at("seating")
    );

    facilities.microphone =
        value.at("microphone")
            .get<std::vector<std::string>>();

    facilities.accessibility =
        value.at("accessibility")
            .get<std::vector<std::string>>();

    facilities.audiovisual =
        value.at("audiovisual")
            .get<std::vector<std::string>>();

    facilities.information_technology =
        value.at("infotechnology")
            .get<std::vector<std::string>>();

    facilities.writing_media =
        value.at("writingMedia")
            .get<std::vector<std::string>>();

    facilities.services =
        value.at("service")
            .get<std::vector<std::string>>();

    return facilities;
}

std::vector<model::Building> load_buildings(
    const std::filesystem::path& path
) {
    const auto json_data = read_json_file(path);

    if (!json_data.is_array()) {
        throw std::runtime_error{
            "buildings.json must contain a JSON array"
        };
    }

    std::vector<model::Building> buildings;
    buildings.reserve(json_data.size());

    for (std::size_t i = 0; i < json_data.size(); ++i) {
        try {
            const auto& value = json_data.at(i);

            model::Building building;

            building.id =
                value.at("id").get<std::string>();

            building.name =
                value.at("name").get<std::string>();

            building.latitude =
                value.at("lat").get<double>();

            building.longitude =
                value.at("long").get<double>();

            building.aliases =
                value.at("aliases")
                    .get<std::vector<std::string>>();

            buildings.push_back(std::move(building));
        }
        catch (const json::exception& error) {
            throw std::runtime_error{
                "Invalid building at index "
                + std::to_string(i)
                + ": "
                + error.what()
            };
        }
    }

    return buildings;
}

std::vector<model::Room> load_rooms(
    const std::filesystem::path& rooms_path,
    const std::filesystem::path& facilities_path
) {
    const auto rooms_json =
        read_json_file(rooms_path);

    const auto facilities_json =
        read_json_file(facilities_path);

    if (!rooms_json.is_array()) {
        throw std::runtime_error{
            "rooms.json must contain a JSON array"
        };
    }

    if (!facilities_json.is_array()) {
        throw std::runtime_error{
            "facilities.json must contain a JSON array"
        };
    }

    if (rooms_json.size() != facilities_json.size()) {
        throw std::runtime_error{
            "rooms.json and facilities.json have different "
            "numbers of entries: "
            + std::to_string(rooms_json.size())
            + " rooms vs "
            + std::to_string(facilities_json.size())
            + " facility records"
        };
    }

    std::vector<model::Room> rooms;
    rooms.reserve(rooms_json.size());

    for (std::size_t i = 0; i < rooms_json.size(); ++i) {
        try {
            const auto& room_json =
                rooms_json.at(i);

            model::Room room;

            room.id =
                room_json.at("id")
                    .get<std::string>();

            if (
                room_json.contains("publishId")
                && !room_json.at("publishId").is_null()
            ) {
                room.publish_id =
                    room_json.at("publishId")
                        .get<std::string>();
            }

            room.name =
                room_json.at("name")
                    .get<std::string>();

            room.abbreviation =
                room_json.at("abbr")
                    .get<std::string>();

            room.usage =
                room_json.at("usage")
                    .get<std::string>();

            room.capacity =
                room_json.at("capacity")
                    .get<int>();

            room.school =
                room_json.at("school")
                    .get<std::string>();

            room.building_id =
                room_json.at("buildingId")
                    .get<std::string>();

            room.latitude =
                room_json.at("lat")
                    .get<double>();

            room.longitude =
                room_json.at("long")
                    .get<double>();

            room.facilities =
                parse_facilities(
                    facilities_json.at(i)
                );

            rooms.push_back(std::move(room));
        }
        catch (const json::exception& error) {
            throw std::runtime_error{
                "Invalid room/facility pair at index "
                + std::to_string(i)
                + ": "
                + error.what()
            };
        }
    }

    return rooms;
}

}

namespace data {

StaticData load_static_data(
    const std::filesystem::path& data_directory
) {
    StaticData result;

    result.buildings = load_buildings(
        data_directory / "buildings.json"
    );

    result.rooms = load_rooms(
        data_directory / "rooms.json",
        data_directory / "facilities.json"
    );

    return result;
}

void validate_static_data(
    const StaticData& static_data
) {
    std::unordered_set<std::string> building_ids;

    for (const auto& building : static_data.buildings) {
        if (!building_ids.insert(building.id).second) {
            throw std::runtime_error{
                "Duplicate building ID: " + building.id
            };
        }
    }

    std::unordered_set<std::string> room_ids;

    for (const auto& room : static_data.rooms) {
        if (!room_ids.insert(room.id).second) {
            throw std::runtime_error{
                "Duplicate room ID: " + room.id
            };
        }

        if (!building_ids.contains(room.building_id)) {
            throw std::runtime_error{
                "Room "
                + room.id
                + " references unknown building "
                + room.building_id
            };
        }

        if (room.capacity < 0) {
            throw std::runtime_error{
                "Room "
                + room.id
                + " has a negative capacity"
            };
        }
    }
}

}
