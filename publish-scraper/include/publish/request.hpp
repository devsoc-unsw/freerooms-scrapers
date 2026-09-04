#pragma once

#include "types/publish.hpp"

#include <string>
#include <vector>

namespace publish {

EventsRequest build_events_request(const ViewOptionsResponse& view_options,
                                   const std::vector<std::string>& location_ids,
                                   int year);

}