#include "rooms/room_id.hpp"

namespace rooms {

std::optional<std::string> extract_room_id(
    const std::string& publish_name
) {
    if (!publish_name.starts_with("K-")) {
        return std::nullopt;
    }

    const auto separator =
        publish_name.find(" - ");

    if (separator == std::string::npos) {
        return std::nullopt;
    }

    return publish_name.substr(
        0,
        separator
    );
}

std::optional<std::string> extract_building_id(
    const std::string& room_id
) {
    if (!room_id.starts_with("K-")) {
        return std::nullopt;
    }

    const auto separator =
        room_id.find(
            '-',
            2
        );

    if (separator == std::string::npos) {
        return std::nullopt;
    }

    return room_id.substr(
        0,
        separator
    );
}

}