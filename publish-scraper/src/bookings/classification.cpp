#include "bookings/classification.hpp"

#include <stdexcept>

namespace bookings {

model::BookingType classify_booking(
    const std::string_view event_type
) {
    if (
        event_type == "Lecture"
        || event_type == "Lecture 1"
        || event_type == "Lecture 2"
    ) {
        return model::BookingType::Lecture;
    }

    if (
        event_type == "Tutorial"
        || event_type == "Tutorial 1"
        || event_type == "Tutorial 2"
    ) {
        return model::BookingType::Tutorial;
    }

    if (
        event_type == "Laboratory"
        || event_type == "Lab 1 of 2"
        || event_type == "Lab 2 of 2"
    ) {
        return model::BookingType::Laboratory;
    }

    if (event_type == "Tut-Lab") {
        return model::BookingType::TutorialLaboratory;
    }

    if (event_type == "Workshop") {
        return model::BookingType::Workshop;
    }

    if (event_type == "Seminar") {
        return model::BookingType::Seminar;
    }

    if (event_type == "Studio") {
        return model::BookingType::Studio;
    }

    if (
        event_type == "Exam"
        || event_type == "*Exam Unit"
    ) {
        return model::BookingType::Exam;
    }

    if (
        event_type == "Clinical"
        || event_type == "Field"
        || event_type == "Honours"
        || event_type == "Project"
        || event_type == "Thesis"
        || event_type == "Web"
        || event_type == "Work"
        || event_type == "*Other class"
    ) {
        return model::BookingType::Class;
    }

    if (
        event_type == "*Booking"
        || event_type == "*Venues & Events"
        || event_type == "BOOK.CONFIRMED"
        || event_type == "BOOK.REQUESTED"
        || event_type == "BOOK.CANCELLED"
    ) {
        return model::BookingType::Miscellaneous;
    }

    if (
        event_type == "Other"
        || event_type == "*Not Used"
    ) {
        return model::BookingType::Other;
    }

    return model::BookingType::Unknown;
}

model::BookingType classify_booking(
    const std::string_view event_type,
    const ParsedBookingName& parsed_name
) {
    const auto publish_type =
        classify_booking(event_type);

    switch (parsed_name.pattern) {
        case NamePattern::OWeek:
            return model::BookingType::Miscellaneous;

        case NamePattern::Block:
            return model::BookingType::Block;

        case NamePattern::Exam:
            return model::BookingType::Exam;

        case NamePattern::InternalSociety:
        case NamePattern::Society:
            return model::BookingType::Society;

        case NamePattern::Internal:
            return model::BookingType::Internal;

        default:
            break;
    }

    const auto publish_is_specific =
        publish_type
            != model::BookingType::Miscellaneous
        && publish_type
            != model::BookingType::Other
        && publish_type
            != model::BookingType::Unknown;

    if (publish_is_specific) {
        return publish_type;
    }

    if (
        parsed_name.pattern
        == NamePattern::Class
    ) {
        return model::BookingType::Class;
    }

    if (
        parsed_name.pattern
        == NamePattern::MiscClass
    ) {
        return model::BookingType::Miscellaneous;
    }

    if (
        publish_type
        == model::BookingType::Unknown
    ) {
        return parsed_name.booking_type;
    }

    return publish_type;
}

std::string_view booking_type_name(
    const model::BookingType booking_type
) {
    switch (booking_type) {
        case model::BookingType::Lecture:
            return "Lecture";

        case model::BookingType::Tutorial:
            return "Tutorial";

        case model::BookingType::Laboratory:
            return "Laboratory";

        case model::BookingType::TutorialLaboratory:
            return "TutorialLaboratory";

        case model::BookingType::Workshop:
            return "Workshop";

        case model::BookingType::Seminar:
            return "Seminar";

        case model::BookingType::Studio:
            return "Studio";

        case model::BookingType::Class:
            return "Class";

        case model::BookingType::Exam:
            return "Exam";

        case model::BookingType::Society:
            return "Society";

        case model::BookingType::Internal:
            return "Internal";

        case model::BookingType::Block:
            return "Block";

        case model::BookingType::Miscellaneous:
            return "Miscellaneous";

        case model::BookingType::Other:
            return "Other";

        case model::BookingType::Unknown:
            return "Unknown";
    }

    return "Unknown";
}

std::string_view booking_type_database_value(
    const model::BookingType booking_type
) {
    switch (booking_type) {
        case model::BookingType::Lecture:
            return "LECTURE";

        case model::BookingType::Tutorial:
            return "TUTORIAL";

        case model::BookingType::Laboratory:
            return "LABORATORY";

        case model::BookingType::TutorialLaboratory:
            return "TUTORIAL_LABORATORY";

        case model::BookingType::Workshop:
            return "WORKSHOP";

        case model::BookingType::Seminar:
            return "SEMINAR";

        case model::BookingType::Studio:
            return "STUDIO";

        case model::BookingType::Class:
            return "CLASS";

        case model::BookingType::Exam:
            return "EXAMS";

        case model::BookingType::Society:
            return "SOCIETY";

        case model::BookingType::Internal:
            return "INTERNAL";

        case model::BookingType::Block:
            return "BLOCK";

        case model::BookingType::Miscellaneous:
            return "MISC";

        case model::BookingType::Other:
            return "OTHER";

        case model::BookingType::Unknown:
            throw std::runtime_error{
                "Cannot serialize Unknown booking type"
            };
    }

    throw std::runtime_error{
        "Invalid booking type"
    };
}

}