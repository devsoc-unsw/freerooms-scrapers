#pragma once

#include "http/client.hpp"
#include "publish/settings.hpp"
#include "types/publish.hpp"

#include <memory>
#include <string>
#include <vector>

namespace publish {

class RequestThrottle;

class Client {
  public:
    explicit Client(http::Client& http_client, RequestSettings settings = {});

    ViewOptionsResponse get_view_options();

    CategoriesResponse get_location_page(int page_number);

    std::vector<Category> get_locations();

    EventsResponse get_events(const std::vector<std::string>& location_ids,
                              const ViewOptionsResponse& view_options,
                              int year);

  private:
    Client(http::Client& http_client,
           RequestSettings settings,
           std::shared_ptr<RequestThrottle> throttle);

    EventsResponse get_events_batch(const std::vector<std::string>& location_ids,
                                    const ViewOptionsResponse& view_options,
                                    int year);

    http::Client& http_client_;
    RequestSettings settings_;
    std::shared_ptr<RequestThrottle> throttle_;
};

} // namespace publish
