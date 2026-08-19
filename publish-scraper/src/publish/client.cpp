#include "publish/client.hpp"

#include "publish/config.hpp"
#include "publish/json.hpp"

#include <stdexcept>
#include <string>

namespace publish {

Client::Client(
    http::Client& http_client
)
    : http_client_{http_client} {
}

ViewOptionsResponse Client::get_view_options() {
    const auto url =
        std::string{config::base_url}
        + "/ViewOptions/"
        + std::string{config::institution_id};

    const auto response =
        http_client_.get(url);

    if (
        response.status_code < 200
        || response.status_code >= 300
    ) {
        throw std::runtime_error{
            "Publish ViewOptions request returned HTTP "
            + std::to_string(response.status_code)
        };
    }

    return parse_view_options(response.body);
}

}