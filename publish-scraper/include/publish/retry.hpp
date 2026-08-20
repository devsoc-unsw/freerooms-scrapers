#pragma once

#include <chrono>

namespace publish::retry {

inline constexpr int max_attempts = 3;
inline constexpr auto initial_delay = std::chrono::milliseconds{500};

constexpr bool is_retryable_status(const long status_code) {
    return status_code == 429 || status_code == 500 || status_code == 502 || status_code == 503 ||
           status_code == 504;
}

constexpr std::chrono::milliseconds backoff_delay(const int failed_attempt) {
    if (failed_attempt <= 0) {
        return std::chrono::milliseconds{0};
    }

    auto delay = initial_delay;

    for (int attempt = 1; attempt < failed_attempt; ++attempt) {
        delay *= 2;
    }

    return delay;
}

} // namespace publish::retry
