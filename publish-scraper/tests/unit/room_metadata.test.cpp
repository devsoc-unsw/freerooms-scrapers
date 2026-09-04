#include "data/room_metadata.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

TEST_CASE("room metadata updater changes matched Publish IDs and preserves unmatched rooms") {
    const auto path =
        std::filesystem::temp_directory_path() / "publish-scraper-room-metadata-test.json";

    {
        std::ofstream output{path};
        output << R"([
            {"id":"K-A1-101","publishId":"old-id","name":"One"},
            {"id":"K-A1-102","publishId":"keep-id","name":"Two"},
            {"id":"K-A1-103","name":"Three"}
        ])";
    }

    const std::vector<rooms::PublishMatch> matches{
        {.room_id = "K-A1-101", .publish_id = "new-id", .publish_name = "K-A1-101 - One"},
        {.room_id = "K-A1-103", .publish_id = "third-id", .publish_name = "K-A1-103 - Three"},
    };

    const auto result = data::write_publish_ids(path, matches);

    CHECK(result.matched_rooms == 2);
    CHECK(result.changed_publish_ids == 2);

    std::ifstream input{path};
    nlohmann::json rooms;
    input >> rooms;

    CHECK(rooms.at(0).at("publishId") == "new-id");
    CHECK(rooms.at(1).at("publishId") == "keep-id");
    CHECK(rooms.at(2).at("publishId") == "third-id");

    std::filesystem::remove(path);
}
