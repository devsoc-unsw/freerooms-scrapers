#pragma once

#include "rooms/publish_mapping.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace data {

struct PublishIdWriteResult {
    std::size_t matched_rooms = 0;
    std::size_t changed_publish_ids = 0;
};

PublishIdWriteResult write_publish_ids(const std::filesystem::path& rooms_path,
                                       const std::vector<rooms::PublishMatch>& matches);

} // namespace data
