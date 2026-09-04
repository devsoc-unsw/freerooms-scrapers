#include "bookings/modules.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

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

std::vector<std::string> split_module_names(const std::string& value) {
    std::vector<std::string> entries;

    std::size_t start = 0;

    while (start < value.size()) {
        const auto separator = value.find(", ", start);

        if (separator == std::string::npos) {
            entries.push_back(trim(std::string_view{value}.substr(start)));

            break;
        }

        entries.push_back(trim(std::string_view{value}.substr(start, separator - start)));

        start = separator + 2;
    }

    return entries;
}

model::Module parse_module_name(const std::string& value) {
    const auto separator = value.find(" - ");

    if (separator == std::string::npos) {
        return model::Module{
            .code = value,
            .name = value,
            .term = std::nullopt,
            .career = std::nullopt,
        };
    }

    const auto code = trim(std::string_view{value}.substr(0, separator));

    const auto details = trim(std::string_view{value}.substr(separator + 3));

    const auto career_separator = details.rfind(' ');

    if (career_separator == std::string::npos) {
        return model::Module{
            .code = code,
            .name = code,
            .term = std::nullopt,
            .career = std::nullopt,
        };
    }

    const auto term = trim(std::string_view{details}.substr(0, career_separator));

    const auto career = trim(std::string_view{details}.substr(career_separator + 1));

    return model::Module{
        .code = code,
        .name = code,
        .term = term,
        .career = career,
    };
}

void apply_module_descriptions(std::vector<model::Module>& modules,
                               const std::string& description) {
    if (modules.empty()) {
        return;
    }

    if (modules.size() == 1) {
        auto& module = modules.front();

        if (!module.term.has_value() || !module.career.has_value()) {
            module.name = trim(description);
            return;
        }
    }

    std::size_t cursor = 0;

    for (auto& module : modules) {
        if (!module.term.has_value() || !module.career.has_value()) {
            continue;
        }

        const auto suffix = " (" + *module.term + " " + *module.career + ")";

        const auto suffix_position = description.find(suffix, cursor);

        if (suffix_position == std::string::npos) {
            continue;
        }

        module.name = trim(std::string_view{description}.substr(cursor, suffix_position - cursor));

        cursor = suffix_position + suffix.size();

        if (description.compare(cursor, 2, ", ") == 0) {
            cursor += 2;
        }
    }
}

} // namespace

namespace bookings {

std::vector<model::Module> parse_modules(const std::optional<std::string>& module_name_raw,
                                         const std::optional<std::string>& module_description_raw) {
    if (!module_name_raw.has_value() || module_name_raw->empty()) {
        return {};
    }

    std::vector<model::Module> modules;

    const auto entries = split_module_names(*module_name_raw);

    modules.reserve(entries.size());

    for (const auto& entry : entries) {
        modules.push_back(parse_module_name(entry));
    }

    if (module_description_raw.has_value()) {
        apply_module_descriptions(modules, *module_description_raw);
    }

    return modules;
}

} // namespace bookings