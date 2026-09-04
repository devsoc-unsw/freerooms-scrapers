#include "publish/request.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

publish::ViewOptionsResponse make_view_options() {
    publish::ViewOptionsResponse options;

    publish::DatePeriod date_period;
    date_period.description = "2026";
    options.date_periods.push_back(date_period);

    publish::TimePeriod time_period;
    time_period.description = "All Day";
    options.time_periods.push_back(time_period);

    return options;
}

publish::Week make_week(const int week_number, const std::string& first_day) {
    publish::Week week;
    week.week_number = week_number;
    week.first_day_in_week = first_day;
    return week;
}

} // namespace

TEST_CASE("Publish weeks overlapping the requested year are included") {
    auto options = make_view_options();

    options.weeks = {
        make_week(0, "2025-12-25T00:00:00"),
        make_week(1, "2025-12-26T00:00:00"),
        make_week(2, "2025-12-29T00:00:00"),
        make_week(3, "2026-01-05T00:00:00"),
        make_week(4, "2026-12-28T00:00:00"),
        make_week(5, "2027-01-04T00:00:00"),
    };

    const auto request =
        publish::build_events_request(options, std::vector<std::string>{"room-id"}, 2026);

    REQUIRE(request.view_options.weeks.size() == 4);

    CHECK(request.view_options.weeks[0].first_day_in_week.starts_with("2025-12-26"));
    CHECK(request.view_options.weeks[1].first_day_in_week.starts_with("2025-12-29"));
    CHECK(request.view_options.weeks[2].first_day_in_week.starts_with("2026-01-05"));
    CHECK(request.view_options.weeks[3].first_day_in_week.starts_with("2026-12-28"));
}