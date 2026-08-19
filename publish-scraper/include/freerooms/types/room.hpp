#pragma once

#include <optional>
#include <string>
#include <vector>

namespace freerooms {

enum class FloorType {
    Flat,
    Tiered,
    Other,
};

enum class SeatingType {
    Movable,
    Fixed,
};

struct RoomFacilities {
    std::optional<FloorType> floor;
    std::optional<SeatingType> seating;

    std::vector<std::string> microphone;
    std::vector<std::string> accessibility;
    std::vector<std::string> audiovisual;
    std::vector<std::string> information_technology;
    std::vector<std::string> writing_media;
    std::vector<std::string> services;
};

struct Room {
    std::string id;

    // UUID used by the Publish API.
    std::optional<std::string> publish_id;

    std::string name;
    std::string abbreviation;
    std::string usage;

    int capacity = 0;

    std::string school;
    std::string building_id;

    double latitude = 0.0;
    double longitude = 0.0;

    RoomFacilities facilities;

    // Metadata obtained from Publish.
    std::optional<std::string> image_url;
    std::optional<std::string> learning_environments_url;
    std::optional<std::string> mazemap_url;
    std::optional<std::string> mazemap_poi;
};

}
