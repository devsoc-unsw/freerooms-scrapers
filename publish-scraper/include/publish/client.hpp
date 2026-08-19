#pragma once

#include "http/client.hpp"
#include "types/publish.hpp"

#include <vector>

namespace publish {

class Client {
public:
    explicit Client(http::Client& http_client);

    ViewOptionsResponse get_view_options();

    CategoriesResponse get_location_page(
        int page_number
    );

    std::vector<Category> get_locations();

    EventsResponse get_events(
        const std::vector<std::string>& location_ids,
        const ViewOptionsResponse& view_options,
        int year
    );

private:
    http::Client& http_client_;
};

}
