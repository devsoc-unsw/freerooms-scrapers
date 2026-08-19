#pragma once

#include <nlohmann/json.hpp>

namespace database {

nlohmann::json build_batch_request(
    nlohmann::json booking_payload,
    nlohmann::json module_payload,
    int year
);

}