#pragma once

#include <optional>
#include <string>
#include <vector>

namespace freerooms {

struct PublishExtraProperty {
    std::string name;
    std::string display_name;
    std::string value;

    int rank = 0;
};

struct PublishEvent {
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

    std::vector<PublishExtraProperty> extra_properties;
};

struct PublishCategory {
    std::string identity;
    std::string name;
};

struct PublishCategoriesResponse {
    int total_pages = 0;

    std::vector<PublishCategory> results;
};

struct PublishCategoryEvents {
    std::string identity;
    std::string name;

    std::vector<PublishEvent> results;
};

struct PublishEventsResponse {
    std::vector<PublishCategoryEvents> category_events;
};

struct PublishDatePeriod {
    std::string description;
    std::string start_date_time;
    std::string end_date_time;

    bool is_default = false;
};

struct PublishDay {
    std::string name;

    int day_of_week = 0;

    bool is_default = false;
};

struct PublishTimePeriod {
    std::string description;
    std::string start_time;
    std::string end_time;

    bool is_default = false;
};

struct PublishWeek {
    int week_number = 0;

    std::string week_label;
    std::string first_day_in_week;
};

struct PublishViewOptionsResponse {
    std::vector<PublishDatePeriod> date_periods;
    std::vector<PublishDay> days;
    std::vector<PublishTimePeriod> time_periods;
    std::vector<PublishWeek> weeks;
};

struct PublishCategorySelection {
    std::string category_type_identity;

    std::vector<std::string> category_identities;
};

struct PublishViewOptionsSelection {
    std::vector<PublishDatePeriod> date_periods;
    std::vector<PublishDay> days;
    std::vector<PublishTimePeriod> time_periods;
    std::vector<PublishWeek> weeks;
};

struct PublishEventsRequest {
    std::vector<PublishCategorySelection> category_types_with_identities;

    PublishViewOptionsSelection view_options;

    bool fetch_bookings = false;
    bool fetch_personal_events = false;

    std::vector<std::string> personal_identities;
};

}
