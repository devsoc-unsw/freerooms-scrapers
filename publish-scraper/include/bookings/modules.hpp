#pragma once

#include "types/booking.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bookings {

std::vector<model::Module> parse_modules(const std::optional<std::string>& module_name_raw,
                                         const std::optional<std::string>& module_description_raw);

}