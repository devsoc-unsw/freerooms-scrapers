#pragma once

#include "http/client.hpp"
#include "types/publish.hpp"

namespace publish {

class Client {
public:
    explicit Client(http::Client& http_client);

    ViewOptionsResponse get_view_options();

    CategoriesResponse get_location_page(
        int page_number
    );

private:
    http::Client& http_client_;
};

}
