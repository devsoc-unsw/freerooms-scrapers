#include "rooms/image_url.hpp"
#include "types/publish.hpp"
#include "types/room.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

model::Room make_room(const std::string& id,
                      const std::string& publish_id,
                      const std::string& name,
                      const std::string& building_id) {
    model::Room room;

    room.id = id;

    room.publish_id = publish_id;

    room.name = name;

    room.building_id = building_id;

    return room;
}

publish::Event make_room_details_event(const std::string& room_details) {
    publish::Event event;

    publish::ExtraProperty property;

    property.name = "Location UserText3";

    property.display_name = "Location UserText3";

    property.value = room_details;

    event.extra_properties.push_back(std::move(property));

    return event;
}

} // namespace

TEST_CASE("room image extraction selects the correct room") {
    auto rooms =
        std::vector<model::Room>{make_room("K-B16-LG01", "publish-lg01", "Colombo LG01", "K-B16"),
                                 make_room("K-B16-LG02", "publish-lg02", "Colombo LG02", "K-B16")};

    const std::string room_details = "[![Colombo LG02]"
                                     "(https://example.com/lg02.jpg =275x*)]"
                                     "(https://www.learningenvironments.unsw.edu.au/"
                                     "physical-spaces/K-B16/K-B16-LG02), "
                                     "[![Colombo LG01]"
                                     "(https://example.com/lg01.jpg =275x*)]"
                                     "(https://www.learningenvironments.unsw.edu.au/"
                                     "physical-spaces/K-B16/K-B16-LG01)";

    const auto lg01_event = make_room_details_event(room_details);

    const auto lg02_event = make_room_details_event(room_details);

    publish::CategoryEvents lg01_category;

    lg01_category.identity = "publish-lg01";

    lg01_category.name = "K-B16-LG01";

    lg01_category.results.push_back(lg01_event);

    publish::CategoryEvents lg02_category;

    lg02_category.identity = "publish-lg02";

    lg02_category.name = "K-B16-LG02";

    lg02_category.results.push_back(lg02_event);

    publish::EventsResponse events;

    events.category_events.push_back(std::move(lg01_category));

    events.category_events.push_back(std::move(lg02_category));

    const auto updated = rooms::apply_publish_image_urls(rooms, events);

    CHECK(updated == 2);

    REQUIRE(rooms.at(0).image_url.has_value());

    REQUIRE(rooms.at(1).image_url.has_value());

    CHECK(rooms.at(0).image_url.value_or("") == "https://example.com/lg01.jpg");

    CHECK(rooms.at(1).image_url.value_or("") == "https://example.com/lg02.jpg");
}

TEST_CASE("room without image remains null") {
    auto rooms =
        std::vector<model::Room>{make_room("K-B16-LG01", "publish-lg01", "Colombo LG01", "K-B16")};

    const auto event = make_room_details_event("[No room image here]");

    publish::CategoryEvents category;

    category.identity = "publish-lg01";

    category.name = "K-B16-LG01";

    category.results.push_back(event);

    publish::EventsResponse events;

    events.category_events.push_back(std::move(category));

    const auto updated = rooms::apply_publish_image_urls(rooms, events);

    CHECK(updated == 0);

    CHECK_FALSE(rooms.at(0).image_url.has_value());
}