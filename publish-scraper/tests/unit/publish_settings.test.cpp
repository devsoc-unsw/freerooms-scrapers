#include "publish/settings.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <optional>
#include <string>

namespace {

class EnvironmentGuard {
  public:
    explicit EnvironmentGuard(const char* name) : name_{name} {
        if (const auto* value = std::getenv(name); value != nullptr) {
            old_value_ = value;
        }
    }

    ~EnvironmentGuard() {
        if (old_value_.has_value()) {
            setenv(name_.c_str(), old_value_->c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }

  private:
    std::string name_;
    std::optional<std::string> old_value_;
};

} // namespace

TEST_CASE("Publish request settings keep current defaults") {
    EnvironmentGuard concurrency_guard{"PUBLISH_MAX_CONCURRENT_REQUESTS"};
    EnvironmentGuard spacing_guard{"PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS"};

    unsetenv("PUBLISH_MAX_CONCURRENT_REQUESTS");
    unsetenv("PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS");

    const auto settings = publish::load_request_settings_from_environment();

    CHECK(settings.max_concurrent_requests == 4);
    CHECK(settings.min_time_between_requests.count() == 0);
}

TEST_CASE("Publish request settings can be overridden") {
    EnvironmentGuard concurrency_guard{"PUBLISH_MAX_CONCURRENT_REQUESTS"};
    EnvironmentGuard spacing_guard{"PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS"};

    setenv("PUBLISH_MAX_CONCURRENT_REQUESTS", "2", 1);
    setenv("PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS", "250", 1);

    const auto settings = publish::load_request_settings_from_environment();

    CHECK(settings.max_concurrent_requests == 2);
    CHECK(settings.min_time_between_requests.count() == 250);
}

TEST_CASE("Publish request concurrency cannot be zero") {
    EnvironmentGuard concurrency_guard{"PUBLISH_MAX_CONCURRENT_REQUESTS"};
    EnvironmentGuard spacing_guard{"PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS"};

    setenv("PUBLISH_MAX_CONCURRENT_REQUESTS", "0", 1);
    unsetenv("PUBLISH_MIN_TIME_MS_BETWEEN_REQUESTS");

    CHECK_THROWS(publish::load_request_settings_from_environment());
}
