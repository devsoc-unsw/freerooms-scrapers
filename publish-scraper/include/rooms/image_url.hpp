#pragma once

#include "types/publish.hpp"
#include "types/room.hpp"

#include <cstddef>
#include <vector>

namespace rooms {

std::size_t apply_publish_image_urls(std::vector<model::Room>& rooms,
                                     const publish::EventsResponse& events);

}