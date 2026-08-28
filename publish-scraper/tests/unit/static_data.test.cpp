#include "data/static_data.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <unordered_set>

TEST_CASE("static room data loads and validates") {
    const auto static_data = data::load_static_data("data");

    REQUIRE_NOTHROW(data::validate_static_data(static_data));

    CHECK(static_data.buildings.size() == 44);
    CHECK(static_data.rooms.size() == 504);
}

TEST_CASE("stored Publish room IDs are non-empty and unique") {
    const auto static_data = data::load_static_data("data");

    std::unordered_set<std::string> publish_ids;
    std::size_t mapped_room_count = 0;

    for (const auto& room : static_data.rooms) {
        if (!room.publish_id.has_value()) {
            continue;
        }

        ++mapped_room_count;

        CHECK_FALSE(room.publish_id->empty());
        CHECK(publish_ids.insert(*room.publish_id).second);
    }

    CHECK(mapped_room_count > 0);
}
