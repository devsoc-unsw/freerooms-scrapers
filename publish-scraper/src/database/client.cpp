#include "database/client.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string require_environment_variable(
    const char* name
) {
    const auto* value =
        std::getenv(name);

    if (
        value == nullptr
        || *value == '\0'
    ) {
        throw std::runtime_error{
            "Missing required environment variable: "
            + std::string{name}
        };
    }

    return value;
}

std::string remove_trailing_slashes(
    std::string value
) {
    while (
        !value.empty()
        && value.back() == '/'
    ) {
        value.pop_back();
    }

    return value;
}

}

namespace database {

Config load_config_from_environment() {
    auto base_url =
        require_environment_variable(
            "HASURAGRES_URL"
        );

    base_url =
        remove_trailing_slashes(
            std::move(base_url)
        );

    if (base_url.empty()) {
        throw std::runtime_error{
            "HASURAGRES_URL is invalid"
        };
    }

    return Config{
        .base_url =
            std::move(base_url),

        .api_key =
            require_environment_variable(
                "HASURAGRES_API_KEY"
            ),
    };
}

Client::Client(
    http::Client& http_client,
    Config config
)
    : http_client_{http_client},
      config_{std::move(config)} {
}

InsertResult Client::batch_insert(
    const nlohmann::json& request
) {
    if (!request.is_array()) {
        throw std::invalid_argument{
            "Hasuragres batch request "
            "must be an array"
        };
    }

    const auto body =
        request.dump();

    const auto response =
        http_client_.post_json(
            config_.base_url
                + "/batch_insert",
            body,
            {
                "X-API-Key: "
                + config_.api_key
            }
        );

    if (
        response.status_code < 200
        || response.status_code >= 300
    ) {
        throw std::runtime_error{
            "Hasuragres batch insert failed "
            "with HTTP "
            + std::to_string(
                response.status_code
            )
            + ": "
            + response.body
        };
    }

    return InsertResult{
        .status_code =
            response.status_code,

        .request_bytes =
            body.size(),

        .response_body =
            response.body,
    };
}

}