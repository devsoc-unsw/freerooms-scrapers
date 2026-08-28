#include "rooms/publish_mapping.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

namespace {

model::Room make_room(std::string id) {
    model::Room room;
    room.id = std::move(id);
    return room;
}

} // namespace

TEST_CASE("Publish location reconciliation reports matches missing rooms and duplicates") {
    const std::vector<model::Room> static_rooms{
        make_room("K-A1-101"),
        make_room("K-A1-102"),
        make_room("K-A1-103"),
    };

    const std::vector<publish::Category> locations{
        {.identity = "publish-101", .name = "K-A1-101 - Room 101"},
        {.identity = "publish-102", .name = "K-A1-102 - Room 102"},
        {.identity = "publish-extra", .name = "K-A1-999 - New Room"},
        {.identity = "publish-duplicate", .name = "K-A1-102 - Duplicate Room"},
        {.identity = "other", .name = "Not a room"},
    };

    const auto report = rooms::match_publish_locations(static_rooms, locations);

    REQUIRE(report.matches.size() == 2);
    CHECK(report.matches.at(0).room_id == "K-A1-101");
    CHECK(report.matches.at(0).publish_id == "publish-101");
    CHECK(report.matches.at(1).room_id == "K-A1-102");

    REQUIRE(report.missing_from_publish.size() == 1);
    CHECK(report.missing_from_publish.at(0) == "K-A1-103");

    REQUIRE(report.missing_from_static.size() == 1);
    CHECK(report.missing_from_static.at(0).identity == "publish-extra");

    REQUIRE(report.unrecognised_publish_locations.size() == 1);
    CHECK(report.unrecognised_publish_locations.at(0).identity == "other");

    REQUIRE(report.duplicate_publish_room_ids.size() == 1);
    CHECK(report.duplicate_publish_room_ids.at(0) == "K-A1-102");
}
