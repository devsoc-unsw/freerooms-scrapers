#include "publish/json.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace {

using json = nlohmann::json;

publish::TimePeriod parse_time_period(
    const json& value
) {
    return publish::TimePeriod{
        .description =
            value.at("Description").get<std::string>(),

        .start_time =
            value.at("StartTime").get<std::string>(),

        .end_time =
            value.at("EndTime").get<std::string>(),

        .is_default =
            value.at("IsDefault").get<bool>(),
    };
}

publish::DatePeriod parse_date_period(
    const json& value
) {
    publish::DatePeriod period{
        .description =
            value.at("Description").get<std::string>(),

        .start_date_time =
            value.at("StartDateTime").get<std::string>(),

        .end_date_time =
            value.at("EndDateTime").get<std::string>(),

        .type = std::nullopt,

        .is_default =
            value.at("IsDefault").get<bool>(),
    };

    if (
        value.contains("Type")
        && !value.at("Type").is_null()
    ) {
        period.type =
            value.at("Type").get<std::string>();
    }

    return period;
}

publish::Week parse_week(
    const json& value
) {
    return publish::Week{
        .week_number =
            value.at("WeekNumber").get<int>(),

        .week_label =
            value.at("WeekLabel").get<std::string>(),

        .first_day_in_week =
            value.at("FirstDayInWeek")
                .get<std::string>(),
    };
}

publish::Day parse_day(
    const json& value
) {
    return publish::Day{
        .name =
            value.at("Name").get<std::string>(),

        .day_of_week =
            value.at("DayOfWeek").get<int>(),

        .is_default =
            value.at("IsDefault").get<bool>(),
    };
}

publish::Category parse_category(
    const json& value
) {
    return publish::Category{
        .identity =
            value.at("Identity")
                .get<std::string>(),

        .name =
            value.at("Name")
                .get<std::string>(),
    };
}

}

namespace publish {

ViewOptionsResponse parse_view_options(
    const std::string& body
) {
    try {
        const auto root = json::parse(body);

        ViewOptionsResponse result;

        for (const auto& value : root.at("TimePeriods")) {
            result.time_periods.push_back(
                parse_time_period(value)
            );
        }

        for (const auto& value : root.at("DatePeriods")) {
            result.date_periods.push_back(
                parse_date_period(value)
            );
        }

        for (const auto& value : root.at("Weeks")) {
            result.weeks.push_back(
                parse_week(value)
            );
        }

        for (const auto& value : root.at("Days")) {
            result.days.push_back(
                parse_day(value)
            );
        }

        return result;
    }
    catch (const json::exception& error) {
        throw std::runtime_error{
            "Invalid Publish ViewOptions response: "
            + std::string{error.what()}
        };
    }
}

CategoriesResponse parse_categories(
    const std::string& body
) {
    try {
        const auto root = json::parse(body);

        CategoriesResponse result;

        result.total_pages =
            root.at("TotalPages").get<int>();

        for (const auto& value : root.at("Results")) {
            result.results.push_back(
                parse_category(value)
            );
        }

        return result;
    }
    catch (const json::exception& error) {
        throw std::runtime_error{
            "Invalid Publish categories response: "
            + std::string{error.what()}
        };
    }
}

}
