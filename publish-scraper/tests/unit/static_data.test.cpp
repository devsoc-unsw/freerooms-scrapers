#include "data/static_data.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>

TEST_CASE(
    "static room data loads and validates"
) {
    const auto static_data =
        data::load_static_data(
            "data"
        );

    REQUIRE_NOTHROW(
        data::validate_static_data(
            static_data
        )
    );

    CHECK(
        static_data.buildings.size()
        == 44
    );

    CHECK(
        static_data.rooms.size()
        == 504
    );
}

TEST_CASE(
    "expected rooms have Publish IDs"
) {
    const auto static_data =
        data::load_static_data(
            "data"
        );

    std::size_t publish_id_count = 0;

    for (
        const auto& room :
        static_data.rooms
    ) {
        if (room.publish_id.has_value()) {
            ++publish_id_count;
        }
    }

    CHECK(
        publish_id_count
        == 487
    );
}