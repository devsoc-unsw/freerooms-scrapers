#include "publish/client.hpp"

#include "publish/config.hpp"
#include "publish/json.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

CategoriesResponse Client::get_location_page(
    const int page_number
) {
    if (page_number < 1) {
        throw std::invalid_argument{
            "Publish page number must be at least 1"
        };
    }

    const auto url =
        std::string{config::base_url}
        + "/CategoryTypes/"
        + std::string{
            config::location_category_type_id
        }
        + "/Categories/FilterWithCache/"
        + std::string{config::institution_id}
        + "?pageNumber="
        + std::to_string(page_number);

    const auto response =
        http_client_.post(url);

    if (
        response.status_code < 200
        || response.status_code >= 300
    ) {
        throw std::runtime_error{
            "Publish location request returned HTTP "
            + std::to_string(response.status_code)
        };
    }

    return parse_categories(response.body);
}

std::vector<Category> Client::get_locations() {
    auto first_page =
        get_location_page(1);

    std::vector<Category> locations;

    locations.reserve(
        first_page.results.size()
        * static_cast<std::size_t>(
            first_page.total_pages
        )
    );

    for (auto& location : first_page.results) {
        locations.push_back(
            std::move(location)
        );
    }

    for (
        int page = 2;
        page <= first_page.total_pages;
        ++page
    ) {
        auto response =
            get_location_page(page);

        for (auto& location : response.results) {
            locations.push_back(
                std::move(location)
            );
        }
    }

    return locations;
}

}