#include "rooms/exclusions.hpp"

#include "rooms/room_id.hpp"

#include <algorithm>
#include <unordered_map>

namespace rooms {

std::optional<ExclusionReason>
get_publish_location_exclusion(const std::string& room_id, const config::Exclusions& exclusions) {
    if (exclusions.virtual_location_ids.contains(room_id)) {
        return ExclusionReason::VirtualLocation;
    }

    if (exclusions.room_ids.contains(room_id)) {
        return ExclusionReason::Room;
    }

    const auto building_id = extract_building_id(room_id);

    if (building_id.has_value() && exclusions.building_ids.contains(*building_id)) {
        return ExclusionReason::Building;
    }

    return std::nullopt;
}

std::optional<ExclusionReason> get_static_room_exclusion(const model::Room& room,
                                                         const config::Exclusions& exclusions) {
    if (const auto location_reason = get_publish_location_exclusion(room.id, exclusions);
        location_reason.has_value()) {
        return location_reason;
    }

    if (exclusions.usages.contains(room.usage)) {
        return ExclusionReason::Usage;
    }

    if (exclusions.schools.contains(room.school)) {
        return ExclusionReason::School;
    }

    return std::nullopt;
}

std::size_t filter_excluded_static_rooms(std::vector<model::Room>& rooms,
                                         const config::Exclusions& exclusions) {
    const auto original_size = rooms.size();

    std::erase_if(rooms, [&](const model::Room& room) {
        return get_static_room_exclusion(room, exclusions).has_value();
    });

    return original_size - rooms.size();
}

std::size_t filter_excluded_publish_locations(std::vector<publish::Category>& locations,
                                              const config::Exclusions& exclusions,
                                              const std::vector<model::Room>& known_rooms) {
    std::unordered_map<std::string, const model::Room*> known_rooms_by_id;
    known_rooms_by_id.reserve(known_rooms.size());

    for (const auto& room : known_rooms) {
        known_rooms_by_id.emplace(room.id, &room);
    }

    const auto original_size = locations.size();

    std::erase_if(locations, [&](const publish::Category& location) {
        const auto room_id = extract_room_id(location.name);

        if (!room_id.has_value()) {
            return false;
        }

        if (const auto room = known_rooms_by_id.find(*room_id); room != known_rooms_by_id.end()) {
            return get_static_room_exclusion(*room->second, exclusions).has_value();
        }

        return get_publish_location_exclusion(*room_id, exclusions).has_value();
    });

    return original_size - locations.size();
}

} // namespace rooms
