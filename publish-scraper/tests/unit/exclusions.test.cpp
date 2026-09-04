#include "rooms/exclusions.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

namespace {

model::Room make_room(std::string id, std::string usage, std::string school) {
    model::Room room;
    room.id = std::move(id);
    room.usage = std::move(usage);
    room.school = std::move(school);
    return room;
}

} // namespace

TEST_CASE("static room exclusions include building room usage and school") {
    config::Exclusions exclusions;
    exclusions.building_ids.insert("K-A1");
    exclusions.room_ids.insert("K-B2-101");
    exclusions.usages.insert("EXAM");
    exclusions.schools.insert("ADMIN");

    model::Room building_room = make_room("K-A1-101", "TUSM", "CSE");
    model::Room room_room = make_room("K-B2-101", "TUSM", "CSE");
    model::Room usage_room = make_room("K-C3-101", "EXAM", "CSE");
    model::Room school_room = make_room("K-D4-101", "TUSM", "ADMIN");
    model::Room included_room = make_room("K-E5-101", "TUSM", "CSE");

    CHECK(rooms::get_static_room_exclusion(building_room, exclusions) ==
          rooms::ExclusionReason::Building);
    CHECK(rooms::get_static_room_exclusion(room_room, exclusions) == rooms::ExclusionReason::Room);
    CHECK(rooms::get_static_room_exclusion(usage_room, exclusions) ==
          rooms::ExclusionReason::Usage);
    CHECK(rooms::get_static_room_exclusion(school_room, exclusions) ==
          rooms::ExclusionReason::School);
    CHECK_FALSE(rooms::get_static_room_exclusion(included_room, exclusions).has_value());
}

TEST_CASE("Publish location exclusions are applied before reconciliation") {
    config::Exclusions exclusions;
    exclusions.building_ids.insert("K-A1");
    exclusions.room_ids.insert("K-B2-101");
    exclusions.virtual_location_ids.insert("K-ONLINE");

    std::vector<publish::Category> locations{
        {.identity = "1", .name = "K-A1-101 - Room"},
        {.identity = "2", .name = "K-B2-101 - Room"},
        {.identity = "3", .name = "K-ONLINE - Online"},
        {.identity = "4", .name = "K-C3-101 - Room"},
        {.identity = "5", .name = "Non-room location"},
    };

    const std::vector<model::Room> known_rooms{
        make_room("K-A1-101", "TUSM", "CSE"),
        make_room("K-B2-101", "TUSM", "CSE"),
        make_room("K-C3-101", "TUSM", "CSE"),
    };

    const auto removed =
        rooms::filter_excluded_publish_locations(locations, exclusions, known_rooms);

    CHECK(removed == 3);
    REQUIRE(locations.size() == 2);
    CHECK(locations.at(0).identity == "4");
    CHECK(locations.at(1).identity == "5");
}
