#pragma once

#include <chrono>
#include <cstddef>

namespace publish {

struct RequestSettings {
    std::size_t max_concurrent_requests = 4;
    std::chrono::milliseconds min_time_between_requests{0};
};

RequestSettings load_request_settings_from_environment();

} // namespace publish
