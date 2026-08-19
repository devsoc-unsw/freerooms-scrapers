#pragma once

#include "types/publish.hpp"

#include <string>

namespace publish {

ViewOptionsResponse parse_view_options(
    const std::string& body
);

}
