#include "publish/request.hpp"

#include "publish/config.hpp"

#include <stdexcept>
#include <string>

namespace publish {

namespace {

bool week_overlaps_year(const Week& week, const int year) {
    if (week.first_day_in_week.size() < 10) {
        return false;
    }

    const auto first_day = week.first_day_in_week.substr(0, 10);
    const auto year_string = std::to_string(year);

    // A week can begin up to six days before January 1 and still overlap
    const auto earliest_start = std::to_string(year - 1) + "-12-26";
    const auto latest_start = year_string + "-12-31";

    return first_day >= earliest_start && first_day <= latest_start;
}

} // namespace

EventsRequest build_events_request(const ViewOptionsResponse& view_options,
                                   const std::vector<std::string>& location_ids,
                                   const int year) {
    if (location_ids.empty()) {
        throw std::invalid_argument{"At least one Publish location is required"};
    }

    if (location_ids.size() > config::category_selection_limit) {
        throw std::invalid_argument{"Too many Publish locations in one request"};
    }

    EventsRequest request;

    request.category_types_with_identities.push_back(CategorySelection{
        .category_type_identity = std::string{config::location_category_type_id},

        .category_identities = location_ids,
    });

    const auto year_string = std::to_string(year);

    // Select the whole requested year.
    for (const auto& period : view_options.date_periods) {
        if (period.description == year_string) {
            request.view_options.date_periods.push_back(period);
        }
    }

    if (request.view_options.date_periods.empty()) {
        throw std::runtime_error{"Publish does not contain date period " + year_string};
    }

    // We want bookings on every day, including weekends
    request.view_options.days = view_options.days;

    // Select Publish's full-day range.
    for (const auto& period : view_options.time_periods) {
        if (period.description == "All Day") {
            request.view_options.time_periods.push_back(period);
        }
    }

    if (request.view_options.time_periods.empty()) {
        throw std::runtime_error{"Publish does not contain the All Day time period"};
    }

    for (const auto& week : view_options.weeks) {
        if (week_overlaps_year(week, year)) {
            request.view_options.weeks.push_back(week);
        }
    }

    if (request.view_options.weeks.empty()) {
        throw std::runtime_error{"Publish contains no weeks for " + year_string};
    }

    request.fetch_bookings = false; // publish booking system
    request.fetch_personal_events = false;
    request.personal_identities.clear();

    return request;
}

} // namespace publish