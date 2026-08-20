#pragma once

#include <nlohmann/json.hpp>

namespace database {

nlohmann::json build_batch_request(nlohmann::json building_payload,
                                   nlohmann::json room_payload,
                                   nlohmann::json booking_payload,
                                   nlohmann::json module_payload,
                                   int year);

}