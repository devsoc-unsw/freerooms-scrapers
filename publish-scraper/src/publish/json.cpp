#include "publish/json.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace {

using json = nlohmann::json;

publish::TimePeriod parse_time_period(const json& value) {
    return publish::TimePeriod{
        .description = value.at("Description").get<std::string>(),

        .start_time = value.at("StartTime").get<std::string>(),

        .end_time = value.at("EndTime").get<std::string>(),

        .is_default = value.at("IsDefault").get<bool>(),
    };
}

publish::DatePeriod parse_date_period(const json& value) {
    publish::DatePeriod period{
        .description = value.at("Description").get<std::string>(),

        .start_date_time = value.at("StartDateTime").get<std::string>(),

        .end_date_time = value.at("EndDateTime").get<std::string>(),

        .type = std::nullopt,

        .is_default = value.at("IsDefault").get<bool>(),
    };

    if (value.contains("Type") && !value.at("Type").is_null()) {
        period.type = value.at("Type").get<std::string>();
    }

    return period;
}

publish::Week parse_week(const json& value) {
    return publish::Week{
        .week_number = value.at("WeekNumber").get<int>(),

        .week_label = value.at("WeekLabel").get<std::string>(),

        .first_day_in_week = value.at("FirstDayInWeek").get<std::string>(),
    };
}

publish::Day parse_day(const json& value) {
    return publish::Day{
        .name = value.at("Name").get<std::string>(),

        .day_of_week = value.at("DayOfWeek").get<int>(),

        .is_default = value.at("IsDefault").get<bool>(),
    };
}

publish::Category parse_category(const json& value) {
    return publish::Category{
        .identity = value.at("Identity").get<std::string>(),

        .name = value.at("Name").get<std::string>(),
    };
}

json serialize_date_period(const publish::DatePeriod& period) {
    json result{{"Description", period.description},
                {"StartDateTime", period.start_date_time},
                {"EndDateTime", period.end_date_time},
                {"IsDefault", period.is_default}};

    if (period.type.has_value()) {
        result["Type"] = *period.type;
    } else {
        result["Type"] = nullptr;
    }

    return result;
}

json serialize_day(const publish::Day& day) {
    return {{"Name", day.name}, {"DayOfWeek", day.day_of_week}, {"IsDefault", day.is_default}};
}

json serialize_time_period(const publish::TimePeriod& period) {
    return {{"Description", period.description},
            {"StartTime", period.start_time},
            {"EndTime", period.end_time},
            {"IsDefault", period.is_default}};
}

json serialize_week(const publish::Week& week) {
    return {{"WeekNumber", week.week_number},
            {"WeekLabel", week.week_label},
            {"FirstDayInWeek", week.first_day_in_week}};
}

std::optional<std::string> parse_optional_string(const json& value, const std::string& key) {
    if (!value.contains(key) || value.at(key).is_null()) {
        return std::nullopt;
    }

    return value.at(key).get<std::string>();
}

publish::ExtraProperty parse_extra_property(const json& value) {
    return publish::ExtraProperty{
        .name = value.at("Name").get<std::string>(),

        .display_name = value.at("DisplayName").get<std::string>(),

        .value = value.at("Value").get<std::string>(),

        .rank = value.value("Rank", 0),
    };
}

publish::Event parse_event(const json& value) {
    publish::Event event;

    event.is_booking = value.at("IsBooking").get<bool>();

    event.is_currently_active = value.at("IsCurrentlyActive").get<bool>();

    event.is_edited = value.at("IsEdited").get<bool>();

    event.is_deleted = value.at("IsDeleted").get<bool>();

    event.user_manually_added_event = value.at("UserManuallyAddedEvent").get<bool>();

    event.event_identity = value.at("EventIdentity").get<std::string>();

    event.identity = value.at("Identity").get<std::string>();

    event.start_date_time = value.at("StartDateTime").get<std::string>();

    event.end_date_time = value.at("EndDateTime").get<std::string>();

    event.name = value.at("Name").get<std::string>();

    event.event_type = value.at("EventType").get<std::string>();

    event.is_published = value.at("IsPublished").get<bool>();

    event.last_modified = value.at("LastModified").get<std::string>();

    event.source = parse_optional_string(value, "Source");

    event.week_ranges = parse_optional_string(value, "WeekRanges");

    event.week_labels = parse_optional_string(value, "WeekLabels");

    if (value.contains("ExtraProperties") && !value.at("ExtraProperties").is_null()) {
        for (const auto& property : value.at("ExtraProperties")) {
            event.extra_properties.push_back(parse_extra_property(property));
        }
    }

    return event;
}

publish::CategoryEvents parse_category_events(const json& value) {
    publish::CategoryEvents result{
        .identity = value.at("Identity").get<std::string>(),

        .name = value.at("Name").get<std::string>(),

        .results = {},
    };

    for (const auto& event : value.at("Results")) {
        result.results.push_back(parse_event(event));
    }

    return result;
}

} // namespace

namespace publish {

ViewOptionsResponse parse_view_options(const std::string& body) {
    try {
        const auto root = json::parse(body);

        ViewOptionsResponse result;

        for (const auto& value : root.at("TimePeriods")) {
            result.time_periods.push_back(parse_time_period(value));
        }

        for (const auto& value : root.at("DatePeriods")) {
            result.date_periods.push_back(parse_date_period(value));
        }

        for (const auto& value : root.at("Weeks")) {
            result.weeks.push_back(parse_week(value));
        }

        for (const auto& value : root.at("Days")) {
            result.days.push_back(parse_day(value));
        }

        return result;
    } catch (const json::exception& error) {
        throw std::runtime_error{"Invalid Publish ViewOptions response: " +
                                 std::string{error.what()}};
    }
}

CategoriesResponse parse_categories(const std::string& body) {
    try {
        const auto root = json::parse(body);

        CategoriesResponse result;

        result.total_pages = root.at("TotalPages").get<int>();

        for (const auto& value : root.at("Results")) {
            result.results.push_back(parse_category(value));
        }

        return result;
    } catch (const json::exception& error) {
        throw std::runtime_error{"Invalid Publish categories response: " +
                                 std::string{error.what()}};
    }
}

std::string serialize_events_request(const EventsRequest& request) {
    json root;

    root["CategoryTypesWithIdentities"] = json::array();

    for (const auto& selection : request.category_types_with_identities) {
        root["CategoryTypesWithIdentities"].push_back(
            {{"CategoryTypeIdentity", selection.category_type_identity},
             {"CategoryIdentities", selection.category_identities}});
    }

    auto& view_options = root["ViewOptions"];

    view_options["DatePeriods"] = json::array();

    for (const auto& period : request.view_options.date_periods) {
        view_options["DatePeriods"].push_back(serialize_date_period(period));
    }

    view_options["Days"] = json::array();

    for (const auto& day : request.view_options.days) {
        view_options["Days"].push_back(serialize_day(day));
    }

    view_options["TimePeriods"] = json::array();

    for (const auto& period : request.view_options.time_periods) {
        view_options["TimePeriods"].push_back(serialize_time_period(period));
    }

    view_options["Weeks"] = json::array();

    for (const auto& week : request.view_options.weeks) {
        view_options["Weeks"].push_back(serialize_week(week));
    }

    root["FetchBookings"] = request.fetch_bookings;

    root["FetchPersonalEvents"] = request.fetch_personal_events;

    root["PersonalIdentities"] = request.personal_identities;

    return root.dump();
}

EventsResponse parse_events(const std::string& body) {
    try {
        const auto root = json::parse(body);

        EventsResponse response;

        for (const auto& category : root.at("CategoryEvents")) {
            response.category_events.push_back(parse_category_events(category));
        }

        return response;
    } catch (const json::exception& error) {
        throw std::runtime_error{"Invalid Publish events response: " + std::string{error.what()}};
    }
}

} // namespace publish
