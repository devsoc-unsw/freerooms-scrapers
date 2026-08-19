#include "rooms/publish_mapping.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

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

}

namespace rooms {

PublishMappingReport match_publish_locations(
    const std::vector<model::Room>& static_rooms,
    const std::vector<publish::Category>& locations
) {
    PublishMappingReport report;

    std::unordered_set<std::string>
        static_room_ids;

    static_room_ids.reserve(
        static_rooms.size()
    );

    for (const auto& room : static_rooms) {
        static_room_ids.insert(room.id);
    }

    std::unordered_map<
        std::string,
        const publish::Category*
    > publish_by_room_id;

    for (const auto& location : locations) {
        const auto room_id =
            extract_room_id(location.name);

        if (!room_id.has_value()) {
            continue;
        }

        const auto [iterator, inserted] =
            publish_by_room_id.emplace(
                *room_id,
                &location
            );

        if (!inserted) {
            report
                .duplicate_publish_room_ids
                .push_back(*room_id);

            continue;
        }

        if (!static_room_ids.contains(*room_id)) {
            report
                .missing_from_static
                .push_back(location);
        }
    }

    for (const auto& room : static_rooms) {
        const auto iterator =
            publish_by_room_id.find(room.id);

        if (iterator == publish_by_room_id.end()) {
            report
                .missing_from_publish
                .push_back(room.id);

            continue;
        }

        const auto& location =
            *iterator->second;

        report.matches.push_back(
            PublishMatch{
                .room_id = room.id,
                .publish_id =
                    location.identity,
                .publish_name =
                    location.name,
            }
        );
    }

    return report;
}

}