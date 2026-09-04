#pragma once

#include <cstddef>
#include <string_view>

namespace publish::config {

inline constexpr std::string_view base_url =
    "https://t1-apac-v4-api-d4-03.azurewebsites.net/api/Public";

inline constexpr std::string_view institution_id = "98c1cede-2447-4c14-92a3-7816107cd42b";

inline constexpr std::string_view location_category_type_id =
    "1e042cb1-547d-41d4-ae93-a1f2c3d34538";

inline constexpr std::size_t category_selection_limit = 20;
inline constexpr std::size_t default_event_request_concurrency = 4;
inline constexpr std::size_t default_min_time_between_requests_ms = 0;

} // namespace publish::config