#include "bookings/name_parser.hpp"

#include <cctype>
#include <regex>
#include <string>
#include <string_view>

namespace {

std::string trim(const std::string_view value) {
    std::size_t start = 0;

    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();

    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return std::string{value.substr(start, end - start)};
}

} // namespace

namespace bookings {

ParsedBookingName parse_booking_name(const std::string_view raw_name) {
    const std::string value{raw_name};

    std::smatch match;

    static const std::regex legacy_oweek_pattern{
        R"(.*Block([^ ]*)O-Week)", std::regex_constants::ECMAScript | std::regex_constants::icase};

    if (std::regex_search(value, match, legacy_oweek_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Miscellaneous,

            .name = "OWeek" + match[1].str(),

            .pattern = NamePattern::OWeek,
        };
    }

    static const std::regex oweek_pattern{R"(^<?\*?BlockO-?week(.*)$)",
                                          std::regex_constants::ECMAScript |
                                              std::regex_constants::icase};

    if (std::regex_match(value, match, oweek_pattern)) {
        auto suffix = trim(match[1].str());

        return ParsedBookingName{
            .booking_type = model::BookingType::Miscellaneous,

            .name = suffix.empty() ? "OWeek" : "OWeek" + suffix,

            .pattern = NamePattern::OWeek,
        };
    }

    static const std::regex block_pattern{
        R"(^<?\*?Block(.+)$)", std::regex_constants::ECMAScript | std::regex_constants::icase};

    if (std::regex_match(value, match, block_pattern)) {
        auto name = trim(match[1].str());

        const auto last_dash = name.rfind('-');

        if (last_dash != std::string::npos) {
            name = name.substr(0, last_dash);
        }

        if (name.empty()) {
            name = "Block";
        }

        return ParsedBookingName{
            .booking_type = model::BookingType::Block,

            .name = name,

            .pattern = NamePattern::Block,
        };
    }

    static const std::regex exam_pattern{
        R"((?:([A-Z]{4}[0-9]{4})\s*)?(Final\s+Exams?|Supp\w*\s+Exams?|Exams)(?:\s+(.*))?)",
        std::regex_constants::ECMAScript | std::regex_constants::icase};

    if (std::regex_search(value, match, exam_pattern)) {
        const auto course = match[1].str();

        const auto reason = match[2].str();

        const auto suffix = match[3].str();

        if (!reason.empty() && !suffix.empty()) {
            return ParsedBookingName{
                .booking_type = model::BookingType::Exam,

                .name = trim(reason + " " + suffix),

                .pattern = NamePattern::Exam,
            };
        }

        if (!course.empty() && !reason.empty()) {
            return ParsedBookingName{
                .booking_type = model::BookingType::Exam,

                .name = course + " " + reason,

                .pattern = NamePattern::Exam,
            };
        }

        return ParsedBookingName{
            .booking_type = model::BookingType::Exam,

            .name = reason.empty() ? "Exams" : reason,

            .pattern = NamePattern::Exam,
        };
    }

    static const std::regex misc_class_pattern{R"(^\*([A-Z]{4}[0-9]{4})-|^FS-([^-].+))"};

    if (std::regex_search(value, match, misc_class_pattern)) {
        if (!match[1].str().empty()) {
            return ParsedBookingName{
                .booking_type = model::BookingType::Miscellaneous,

                .name = match[1].str(),

                .pattern = NamePattern::MiscClass,
            };
        }

        return ParsedBookingName{
            .booking_type = model::BookingType::Miscellaneous,

            .name = "FS-" + match[2].str(),

            .pattern = NamePattern::MiscClass,
        };
    }

    static const std::regex class_pattern{R"(([A-Z]{4}[0-9]{4})-.*-([A-Z0-9]{3}))"};

    if (std::regex_search(value, match, class_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Class,

            .name = match[1].str() + " " + match[2].str(),

            .pattern = NamePattern::Class,
        };
    }

    static const std::regex internal_society_pattern{R"(^\*\d{8}-ARCSG([A-Z0-9]+)-)"};

    if (std::regex_search(value, match, internal_society_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Society,

            .name = match[1].str(),

            .pattern = NamePattern::InternalSociety,
        };
    }

    static const std::regex society_pattern{R"(^\*\d{8}-ARC([A-Z0-9]+)-)"};

    if (std::regex_search(value, match, society_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Society,

            .name = match[1].str(),

            .pattern = NamePattern::Society,
        };
    }

    static const std::regex sg_society_pattern{R"(^SG-(.+)-[0-9]+[A-Za-z]?$)",
                                               std::regex_constants::ECMAScript |
                                                   std::regex_constants::icase};

    if (std::regex_match(value, match, sg_society_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Society,

            .name = trim(match[1].str()),

            .pattern = NamePattern::Society,
        };
    }

    static const std::regex internal_pattern{R"(^\*\d{8}-([A-Z]+))"};

    if (std::regex_search(value, match, internal_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Internal,

            .name = match[1].str(),

            .pattern = NamePattern::Internal,
        };
    }

    static const std::regex misc_pattern{R"(^.{2}-([^-]+))"};

    if (std::regex_search(value, match, misc_pattern)) {
        return ParsedBookingName{
            .booking_type = model::BookingType::Miscellaneous,

            .name = trim(match[1].str()),

            .pattern = NamePattern::Misc,
        };
    }

    return ParsedBookingName{
        .booking_type = model::BookingType::Miscellaneous,

        .name = trim(value),

        .pattern = NamePattern::Misc,
    };
}

} // namespace bookings