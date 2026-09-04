#include "publish/client.hpp"

#include "publish/config.hpp"
#include "publish/json.hpp"
#include "publish/request.hpp"
#include "publish/retry.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace publish {

class RequestThrottle {
  public:
    explicit RequestThrottle(const std::chrono::milliseconds minimum_interval)
        : minimum_interval_{minimum_interval} {}

    void wait_for_slot() {
        if (minimum_interval_.count() == 0) {
            return;
        }

        const std::unique_lock lock{mutex_};

        const auto now = std::chrono::steady_clock::now();

        if (next_request_time_ > now) {
            std::this_thread::sleep_until(next_request_time_);
        }

        next_request_time_ = std::chrono::steady_clock::now() + minimum_interval_;
    }

  private:
    std::chrono::milliseconds minimum_interval_;
    std::mutex mutex_;
    std::chrono::steady_clock::time_point next_request_time_{};
};

namespace {

template <typename Request>
http::Response perform_publish_request(Request&& request, RequestThrottle& throttle) {
    for (int attempt = 1; attempt <= retry::max_attempts; ++attempt) {
        throttle.wait_for_slot();

        try {
            auto response = request();

            if (!retry::is_retryable_status(response.status_code) ||
                attempt == retry::max_attempts) {
                return response;
            }
        } catch (const std::runtime_error&) {
            if (attempt == retry::max_attempts) {
                throw;
            }
        }

        std::this_thread::sleep_for(retry::backoff_delay(attempt));
    }

    throw std::runtime_error{"Publish request retry loop ended unexpectedly"};
}

} // namespace

Client::Client(http::Client& http_client, RequestSettings settings)
    : Client{http_client,
             settings,
             std::make_shared<RequestThrottle>(settings.min_time_between_requests)} {}

Client::Client(http::Client& http_client,
               RequestSettings settings,
               std::shared_ptr<RequestThrottle> throttle)
    : http_client_{http_client}, settings_{settings}, throttle_{std::move(throttle)} {
    if (settings_.max_concurrent_requests == 0) {
        throw std::invalid_argument{"Publish request concurrency must be at least 1"};
    }
}

ViewOptionsResponse Client::get_view_options() {
    const auto url =
        std::string{config::base_url} + "/ViewOptions/" + std::string{config::institution_id};

    const auto response =
        perform_publish_request([&] { return http_client_.get(url); }, *throttle_);

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error{"Publish ViewOptions request returned HTTP " +
                                 std::to_string(response.status_code)};
    }

    return parse_view_options(response.body);
}

CategoriesResponse Client::get_location_page(const int page_number) {
    if (page_number < 1) {
        throw std::invalid_argument{"Publish page number must be at least 1"};
    }

    const auto url = std::string{config::base_url} + "/CategoryTypes/" +
                     std::string{config::location_category_type_id} +
                     "/Categories/FilterWithCache/" + std::string{config::institution_id} +
                     "?pageNumber=" + std::to_string(page_number);

    const auto response =
        perform_publish_request([&] { return http_client_.post(url); }, *throttle_);

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error{"Publish location request returned HTTP " +
                                 std::to_string(response.status_code)};
    }

    return parse_categories(response.body);
}

std::vector<Category> Client::get_locations() {
    auto first_page = get_location_page(1);

    std::vector<Category> locations;

    locations.reserve(first_page.results.size() * static_cast<std::size_t>(first_page.total_pages));

    for (auto& location : first_page.results) {
        locations.push_back(std::move(location));
    }

    for (int page = 2; page <= first_page.total_pages; ++page) {
        auto response = get_location_page(page);

        for (auto& location : response.results) {
            locations.push_back(std::move(location));
        }
    }

    return locations;
}

EventsResponse Client::get_events_batch(const std::vector<std::string>& location_ids,
                                        const ViewOptionsResponse& view_options,
                                        const int year) {
    const auto request = build_events_request(view_options, location_ids, year);

    const auto body = serialize_events_request(request);

    const auto url = std::string{config::base_url} +
                     "/CategoryTypes/Categories/"
                     "Events/Filter/" +
                     std::string{config::institution_id};

    const auto response =
        perform_publish_request([&] { return http_client_.post_json(url, body); }, *throttle_);

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error{"Publish events request returned HTTP " +
                                 std::to_string(response.status_code)};
    }

    return parse_events(response.body);
}

EventsResponse Client::get_events(const std::vector<std::string>& location_ids,
                                  const ViewOptionsResponse& view_options,
                                  const int year) {
    if (location_ids.empty()) {
        throw std::invalid_argument{"At least one Publish location is required"};
    }

    const auto batch_count = (location_ids.size() + config::category_selection_limit - 1) /
                             config::category_selection_limit;

    std::vector<EventsResponse> batch_responses(batch_count);

    std::atomic<std::size_t> next_batch{0};
    std::atomic<bool> stop{false};

    std::mutex exception_mutex;
    std::exception_ptr worker_exception;

    const auto worker = [&] {
        try {
            http::Client worker_http_client;
            Client worker_client{worker_http_client, settings_, throttle_};

            while (!stop.load(std::memory_order_relaxed)) {
                const auto batch_index = next_batch.fetch_add(1, std::memory_order_relaxed);

                if (batch_index >= batch_count) {
                    return;
                }

                const auto start = batch_index * config::category_selection_limit;
                const auto end =
                    std::min(start + config::category_selection_limit, location_ids.size());

                std::vector<std::string> batch;
                batch.reserve(end - start);

                for (std::size_t index = start; index < end; ++index) {
                    batch.push_back(location_ids[index]);
                }

                batch_responses[batch_index] =
                    worker_client.get_events_batch(batch, view_options, year);
            }
        } catch (...) {
            stop.store(true, std::memory_order_relaxed);

            const std::lock_guard lock{exception_mutex};

            if (worker_exception == nullptr) {
                worker_exception = std::current_exception();
            }
        }
    };

    const auto worker_count = std::min(settings_.max_concurrent_requests, batch_count);

    std::vector<std::jthread> workers;
    workers.reserve(worker_count);

    for (std::size_t index = 0; index < worker_count; ++index) {
        workers.emplace_back(worker);
    }

    for (auto& thread : workers) {
        thread.join();
    }

    if (worker_exception != nullptr) {
        std::rethrow_exception(worker_exception);
    }

    std::size_t category_count = 0;

    for (const auto& response : batch_responses) {
        category_count += response.category_events.size();
    }

    EventsResponse combined_response;
    combined_response.category_events.reserve(category_count);

    for (auto& response : batch_responses) {
        for (auto& category : response.category_events) {
            combined_response.category_events.push_back(std::move(category));
        }
    }

    return combined_response;
}

} // namespace publish
