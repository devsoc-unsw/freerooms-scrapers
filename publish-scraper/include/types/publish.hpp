#pragma once

#include <optional>
#include <string>
#include <vector>

namespace publish {

struct ExtraProperty {
    std::string name;
    std::string display_name;
    std::string value;

    int rank = 0;
};

struct Event {
    bool is_booking = false;
    bool is_currently_active = false;
    bool is_edited = false;
    bool is_deleted = false;
    bool user_manually_added_event = false;

    // Shared across recurring occurrences.
    std::string event_identity;

    // Unique occurrence identity.
    std::string identity;

    std::string start_date_time;
    std::string end_date_time;

    std::string name;
    std::string event_type;

    bool is_published = false;

    std::string last_modified;

    std::optional<std::string> source;
    std::optional<std::string> week_ranges;
    std::optional<std::string> week_labels;

    std::vector<ExtraProperty> extra_properties;
};

struct Category {
    std::string identity;
    std::string name;
};

struct CategoriesResponse {
    int total_pages = 0;

    std::vector<Category> results;
};

struct CategoryEvents {
    std::string identity;
    std::string name;

    std::vector<Event> results;
};

struct EventsResponse {
    std::vector<CategoryEvents> category_events;
};

struct DatePeriod {
    std::string description;
    std::string start_date_time;
    std::string end_date_time;

    std::optional<std::string> type;

    bool is_default = false;
};

struct Day {
    std::string name;
    int day_of_week = 0;
    bool is_default = false;
};

struct TimePeriod {
    std::string description;
    std::string start_time;
    std::string end_time;
    bool is_default = false;
};

struct Week {
    int week_number = 0;
    std::string week_label;
    std::string first_day_in_week;
};

struct ViewOptionsResponse {
    std::vector<TimePeriod> time_periods;
    std::vector<DatePeriod> date_periods;
    std::vector<Week> weeks;
    std::vector<Day> days;
};

struct CategorySelection {
    std::string category_type_identity;

    std::vector<std::string> category_identities;
};

struct ViewOptionsSelection {
    std::vector<DatePeriod> date_periods;
    std::vector<Day> days;
    std::vector<TimePeriod> time_periods;
    std::vector<Week> weeks;
};

struct EventsRequest {
    std::vector<CategorySelection> category_types_with_identities;

    ViewOptionsSelection view_options;

    bool fetch_bookings = false;
    bool fetch_personal_events = false;

    std::vector<std::string> personal_identities;
};

} // namespace publish
