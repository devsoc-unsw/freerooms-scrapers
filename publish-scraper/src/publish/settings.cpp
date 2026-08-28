#include "publish/settings.hpp"

#include "publish/config.hpp"

#include <charconv>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::size_t load_size_t_environment(const char* name,
                                    const std::size_t default_value,
                                    const std::size_t minimum_value) {
    const auto* value = std::getenv(name);

    if (value == nullptr || *value == '\0') {
        return default_value;
    }

    const std::string text{value};

    unsigned long long parsed = 0;

    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        parsed < minimum_value || parsed > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error{"Invalid " + std::string{name} + " value: " + text};
    }

    return static_cast<std::size_t>(parsed);
}

} // namespace

namespace publish {

RequestSettings load_request_settings_from_environment() {
    const auto max_concurrent_requests = load_size_t_environment(
        "PUBLISH_MAX_CONCURRENT_REQUESTS", config::default_event_request_concurrency, 1);

    const auto min_time_ms = load_size_t_environment(
        "PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS", config::default_min_time_between_requests_ms, 0);

    using MillisecondsRep = std::chrono::milliseconds::rep;

    if (min_time_ms > static_cast<std::size_t>(std::numeric_limits<MillisecondsRep>::max())) {
        throw std::runtime_error{"PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS is too large"};
    }

    return RequestSettings{
        .max_concurrent_requests = max_concurrent_requests,
        .min_time_between_requests =
            std::chrono::milliseconds{static_cast<MillisecondsRep>(min_time_ms)},
    };
}

} // namespace publish
