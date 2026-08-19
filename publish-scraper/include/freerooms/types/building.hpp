#pragma once

#include <string>
#include <vector>

namespace freerooms {

struct Building {
    std::string id;
    std::string name;

    double latitude = 0.0;
    double longitude = 0.0;

    std::vector<std::string> aliases;
};

}
