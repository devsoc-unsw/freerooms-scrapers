#pragma once

#include "rooms/publish_mapping.hpp"

#include <filesystem>
#include <vector>

namespace data {

void write_publish_ids(
    const std::filesystem::path& rooms_path,
    const std::vector<rooms::PublishMatch>& matches
);

}