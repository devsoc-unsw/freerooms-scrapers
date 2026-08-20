#include "database/static_json.hpp"
#include "types/room.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

TEST_CASE(
    "room image URL is serialized for database"
) {
    model::Room room;

    room.id =
        "K-B16-LG01";

    room.name =
        "Colombo LG01";

    room.abbreviation =
        "Col LG01";

    room.usage =
        "TUSM";

    room.capacity =
        48;

    room.school =
        " ";

    room.building_id =
        "K-B16";

    room.latitude =
        -33.916155;

    room.longitude =
        151.231302;

    room.image_url =
        "https://example.com/room.jpg";

    const auto result =
        database::serialize_rooms(
            {room}
        );

    REQUIRE(
        result.size() == 1
    );

    CHECK(
        result.at(0)
            .at("imageUrl")
        == "https://example.com/room.jpg"
    );
}

TEST_CASE(
    "room without image serializes null"
) {
    model::Room room;

    room.id =
        "K-B16-LG01";

    room.name =
        "Colombo LG01";

    room.abbreviation =
        "Col LG01";

    room.usage =
        "TUSM";

    room.school =
        " ";

    room.building_id =
        "K-B16";

    const auto result =
        database::serialize_rooms(
            {room}
        );

    REQUIRE(
        result.size() == 1
    );

    CHECK(
        result.at(0)
            .at("imageUrl")
            .is_null()
    );
}