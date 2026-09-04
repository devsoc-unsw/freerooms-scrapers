#include "publish/config.hpp"
#include "publish/retry.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>

TEST_CASE("Publish retry policy retries rate limiting") {
    CHECK(publish::retry::is_retryable_status(429));
}

TEST_CASE("Publish retry policy retries transient server errors") {
    CHECK(publish::retry::is_retryable_status(500));
    CHECK(publish::retry::is_retryable_status(502));
    CHECK(publish::retry::is_retryable_status(503));
    CHECK(publish::retry::is_retryable_status(504));
}

TEST_CASE("Publish retry policy does not retry successful responses") {
    CHECK_FALSE(publish::retry::is_retryable_status(200));
    CHECK_FALSE(publish::retry::is_retryable_status(201));
    CHECK_FALSE(publish::retry::is_retryable_status(204));
}

TEST_CASE("Publish retry policy does not retry permanent client errors") {
    CHECK_FALSE(publish::retry::is_retryable_status(400));
    CHECK_FALSE(publish::retry::is_retryable_status(401));
    CHECK_FALSE(publish::retry::is_retryable_status(403));
    CHECK_FALSE(publish::retry::is_retryable_status(404));
}

TEST_CASE("Publish retry policy does not retry other server errors") {
    CHECK_FALSE(publish::retry::is_retryable_status(501));
}

TEST_CASE("Publish retry backoff starts at 500 ms and doubles") {
    CHECK(publish::retry::backoff_delay(1) == std::chrono::milliseconds{500});
    CHECK(publish::retry::backoff_delay(2) == std::chrono::milliseconds{1000});
}

TEST_CASE("Publish retry attempts remain bounded") {
    CHECK(publish::retry::max_attempts == 3);
}

TEST_CASE("Publish request defaults preserve the current request behaviour") {
    CHECK(publish::config::default_event_request_concurrency == 4);
    CHECK(publish::config::default_min_time_between_requests_ms == 0);
}
