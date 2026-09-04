#pragma once

#include <optional>
#include <string>

namespace rooms {

std::optional<std::string> extract_room_id(const std::string& publish_name);

std::optional<std::string> extract_building_id(const std::string& room_id);

} // namespace rooms