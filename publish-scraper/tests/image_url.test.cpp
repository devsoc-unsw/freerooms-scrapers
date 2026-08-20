#include "rooms/image_url.hpp"
#include "types/publish.hpp"
#include "types/room.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

TEST_CASE(
    "room image extraction selects the correct room"
) {
    auto rooms =
        std::vector<model::Room>{
            model::Room{
                .id = "K-B16-LG01",
                .publish_id = "publish-lg01",
                .name = "Colombo LG01",
                .building_id = "K-B16",
            },
            model::Room{
                .id = "K-B16-LG02",
                .publish_id = "publish-lg02",
                .name = "Colombo LG02",
                .building_id = "K-B16",
            }
        };

    const std::string room_details =
        "[![Colombo LG02]"
        "(https://example.com/lg02.jpg =275x*)]"
        "(https://www.learningenvironments.unsw.edu.au/"
        "physical-spaces/K-B16/K-B16-LG02), "
        "[![Colombo LG01]"
        "(https://example.com/lg01.jpg =275x*)]"
        "(https://www.learningenvironments.unsw.edu.au/"
        "physical-spaces/K-B16/K-B16-LG01)";

    publish::Event lg01_event;

    lg01_event.extra_properties.push_back(
        publish::ExtraProperty{
            .name =
                "Location UserText3",
            .display_name =
                "Location UserText3",
            .value =
                room_details,
        }
    );

    publish::Event lg02_event =
        lg01_event;

    publish::EventsResponse events;

    events.category_events.push_back(
        publish::CategoryEvents{
            .identity =
                "publish-lg01",
            .name =
                "K-B16-LG01",
            .results =
                {lg01_event},
        }
    );

    events.category_events.push_back(
        publish::CategoryEvents{
            .identity =
                "publish-lg02",
            .name =
                "K-B16-LG02",
            .results =
                {lg02_event},
        }
    );

    const auto updated =
        rooms::apply_publish_image_urls(
            rooms,
            events
        );

    CHECK(updated == 2);

    REQUIRE(
        rooms.at(0)
            .image_url
            .has_value()
    );

    REQUIRE(
        rooms.at(1)
            .image_url
            .has_value()
    );

    CHECK(
        *rooms.at(0).image_url
        == "https://example.com/lg01.jpg"
    );

    CHECK(
        *rooms.at(1).image_url
        == "https://example.com/lg02.jpg"
    );
}

TEST_CASE(
    "room without image remains null"
) {
    auto rooms =
        std::vector<model::Room>{
            model::Room{
                .id = "K-B16-LG01",
                .publish_id = "publish-lg01",
                .name = "Colombo LG01",
                .building_id = "K-B16",
            }
        };

    publish::Event event;

    event.extra_properties.push_back(
        publish::ExtraProperty{
            .name =
                "Location UserText3",
            .display_name =
                "Location UserText3",
            .value =
                "[No room image here]"
        }
    );

    publish::EventsResponse events;

    events.category_events.push_back(
        publish::CategoryEvents{
            .identity =
                "publish-lg01",
            .name =
                "K-B16-LG01",
            .results =
                {event},
        }
    );

    const auto updated =
        rooms::apply_publish_image_urls(
            rooms,
            events
        );

    CHECK(updated == 0);

    CHECK_FALSE(
        rooms.at(0)
            .image_url
            .has_value()
    );
}