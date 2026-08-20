#include "rooms/image_url.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

std::optional<std::string> find_extra_property(
    const publish::Event& event,
    const std::string& name
) {
    for (
        const auto& property :
        event.extra_properties
    ) {
        if (property.name == name) {
            return property.value;
        }
    }

    return std::nullopt;
}

std::optional<std::string> extract_image_url(
    const std::string& markdown,
    const model::Room& room
) {
    const auto details_path =
        "/physical-spaces/"
        + room.building_id
        + "/"
        + room.id;

    const auto details_position =
        markdown.find(details_path);

    if (details_position == std::string::npos) {
        return std::nullopt;
    }

    const auto entry_start =
        markdown.rfind(
            "[![",
            details_position
        );

    if (entry_start == std::string::npos) {
        return std::nullopt;
    }

    const auto image_marker =
        markdown.find(
            "](",
            entry_start
        );

    if (
        image_marker == std::string::npos
        || image_marker >= details_position
    ) {
        return std::nullopt;
    }

    const auto image_start =
        image_marker + 2;

    const auto image_end =
        markdown.find_first_of(
            " )",
            image_start
        );

    if (
        image_end == std::string::npos
        || image_end >= details_position
        || image_end == image_start
    ) {
        return std::nullopt;
    }

    return markdown.substr(
        image_start,
        image_end - image_start
    );
}

}

namespace rooms {

std::size_t apply_publish_image_urls(
    std::vector<model::Room>& rooms,
    const publish::EventsResponse& events
) {
    std::unordered_map<
        std::string,
        model::Room*
    > rooms_by_publish_id;

    for (auto& room : rooms) {
        if (!room.publish_id.has_value()) {
            continue;
        }

        const auto inserted =
            rooms_by_publish_id
                .emplace(
                    *room.publish_id,
                    &room
                )
                .second;

        if (!inserted) {
            throw std::runtime_error{
                "Duplicate Publish room ID: "
                + *room.publish_id
            };
        }
    }

    std::size_t updated_count = 0;

    for (
        const auto& category :
        events.category_events
    ) {
        const auto room_it =
            rooms_by_publish_id.find(
                category.identity
            );

        if (
            room_it
            == rooms_by_publish_id.end()
        ) {
            continue;
        }

        auto& room =
            *room_it->second;

        for (
            const auto& event :
            category.results
        ) {
            const auto room_details =
                find_extra_property(
                    event,
                    "Location UserText3"
                );

            if (!room_details.has_value()) {
                continue;
            }

            const auto image_url =
                extract_image_url(
                    *room_details,
                    room
                );

            if (!image_url.has_value()) {
                continue;
            }

            room.image_url =
                *image_url;

            ++updated_count;

            break;
        }
    }

    return updated_count;
}

}