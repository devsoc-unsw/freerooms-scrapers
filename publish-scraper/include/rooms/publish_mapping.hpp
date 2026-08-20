#pragma once

#include "types/publish.hpp"
#include "types/room.hpp"

#include <string>
#include <vector>

namespace rooms {

struct PublishMatch {
    std::string room_id;
    std::string publish_id;
    std::string publish_name;
};

struct PublishMappingReport {
    std::vector<PublishMatch> matches;

    std::vector<std::string> missing_from_publish;

    std::vector<publish::Category> missing_from_static;

    std::vector<std::string> duplicate_publish_room_ids;
};

PublishMappingReport match_publish_locations(const std::vector<model::Room>& rooms,
                                             const std::vector<publish::Category>& locations);

} // namespace rooms