#pragma once

#include "types/publish.hpp"

#include <string>

namespace publish {

ViewOptionsResponse parse_view_options(const std::string& body);

CategoriesResponse parse_categories(const std::string& body);

std::string serialize_events_request(const EventsRequest& request);

EventsResponse parse_events(const std::string& body);

} // namespace publish
