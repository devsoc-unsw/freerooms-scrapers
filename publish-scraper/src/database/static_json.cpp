#include "database/static_json.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

nlohmann::json floor_value(
    const std::optional<model::FloorType>& floor
) {
    if (!floor.has_value()) {
        return nullptr;
    }

    switch (*floor) {
        case model::FloorType::Flat:
            return "Flat";

        case model::FloorType::Tiered:
            return "Tiered";

        case model::FloorType::Other:
            return "Other";
    }

    throw std::runtime_error{
        "Unknown floor type"
    };
}

nlohmann::json seating_value(
    const std::optional<model::SeatingType>& seating
) {
    if (!seating.has_value()) {
        return nullptr;
    }

    switch (*seating) {
        case model::SeatingType::Movable:
            return "Movable";

        case model::SeatingType::Fixed:
            return "Fixed";
    }

    throw std::runtime_error{
        "Unknown seating type"
    };
}

}

namespace database {

nlohmann::json serialize_buildings(
    const std::vector<model::Building>& buildings,
    const std::vector<model::Room>& rooms
) {
    std::unordered_set<std::string>
        used_building_ids;

    for (const auto& room : rooms) {
        used_building_ids.insert(
            room.building_id
        );
    }

    auto result =
        nlohmann::json::array();

    for (const auto& building : buildings) {
        if (
            !used_building_ids.contains(
                building.id
            )
        ) {
            continue;
        }

        auto row =
            nlohmann::json::object();

        row["id"] =
            building.id;

        row["name"] =
            building.name;

        row["lat"] =
            building.latitude;

        row["long"] =
            building.longitude;

        row["aliases"] =
            building.aliases;

        result.push_back(
            std::move(row)
        );
    }

    return result;
}

nlohmann::json serialize_rooms(
    const std::vector<model::Room>& rooms
) {
    auto result =
        nlohmann::json::array();

    auto& array =
        result.get_ref<
            nlohmann::json::array_t&
        >();

    array.reserve(
        rooms.size()
    );

    for (const auto& room : rooms) {
        auto row =
            nlohmann::json::object();

        row["abbr"] =
            room.abbreviation;

        row["name"] =
            room.name;

        row["id"] =
            room.id;

        row["usage"] =
            room.usage;

        row["capacity"] =
            room.capacity;

        row["school"] =
            room.school;

        row["buildingId"] =
            room.building_id;

        row["floor"] =
            floor_value(
                room.facilities.floor
            );

        row["seating"] =
            seating_value(
                room.facilities.seating
            );

        row["microphone"] =
            room.facilities.microphone;

        row["accessibility"] =
            room.facilities.accessibility;

        row["audiovisual"] =
            room.facilities.audiovisual;

        row["infotechnology"] =
            room.facilities
                .information_technology;

        row["writingMedia"] =
            room.facilities
                .writing_media;

        row["service"] =
            room.facilities.services;

        row["lat"] =
            room.latitude;

        row["long"] =
            room.longitude;

        result.push_back(
            std::move(row)
        );
    }

    return result;
}

}