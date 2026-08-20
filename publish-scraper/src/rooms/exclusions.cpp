#include "rooms/exclusions.hpp"

#include "rooms/room_id.hpp"

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

} // namespace rooms