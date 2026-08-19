#pragma once

#include <optional>
#include <string>
#include <vector>

namespace model {

enum class BookingType {
    Lecture,
    Tutorial,
    Laboratory,
    TutorialLaboratory,
    Workshop,
    Seminar,
    Studio,
    Class,
    Exam,
    Society,
    Internal,
    Block,
    Library,
    Miscellaneous,
    Other,
    Unknown,
};

struct Module {
    std::string code;
    std::string name;

    std::optional<std::string> term;
    std::optional<std::string> career;
};

struct Booking {
    std::string room_id;

    // Publish Identity:
    // one actual occurrence on one date.
    std::string occurrence_id;

    // Publish EventIdentity:
    // shared by occurrences of the recurring activity.
    std::string event_id;

    std::string start;
    std::string end;

    // Friendly name
    std::string name;

    // Exact upstream Publish name.
    std::string raw_name;

    // Freerooms type
    BookingType booking_type = BookingType::Unknown;

    // Exact Publish EventType string.
    std::string event_type;

    // A booking may represent multiple courses.
    std::vector<Module> modules;

    // Preserve the original API values even after parsing modules.
    std::optional<std::string>
        module_name_raw;

    std::optional<std::string>
        module_description_raw;

    // How many people they planned for
    std::optional<int> planned_size;

    std::optional<std::string> week_labels;
    std::optional<std::string> week_pattern;

    std::optional<std::string> source;
    std::optional<std::string> last_modified;

    bool is_booking = false;
    bool is_published = false;
    bool is_deleted = false;
};

}
