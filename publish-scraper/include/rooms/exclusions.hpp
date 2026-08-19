#pragma once

#include "config/exclusions.hpp"

#include <optional>
#include <string>

namespace rooms {

enum class ExclusionReason {
    Building,
    Room,
    VirtualLocation,
};

std::optional<ExclusionReason>
get_publish_location_exclusion(
    const std::string& room_id,
    const config::Exclusions& exclusions
);

}