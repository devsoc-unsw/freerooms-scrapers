#pragma once

#include "http/client.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>

namespace database {

struct Config {
    std::string base_url;
    std::string api_key;
};

struct InsertResult {
    long status_code = 0;
    std::size_t request_bytes = 0;
    std::string response_body;
};

Config load_config_from_environment();

class Client {
  public:
    Client(http::Client& http_client, Config config);

    InsertResult batch_insert(const nlohmann::json& request);

  private:
    http::Client& http_client_;
    Config config_;
};

} // namespace database