#include "database/request.hpp"

#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

TEST_CASE("database batch uses correct table order") {
    const auto result = database::build_batch_request(nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      2026);

    REQUIRE(result.size() == 4);

    CHECK(result.at(0).at("metadata").at("table_name") == "Buildings");

    CHECK(result.at(1).at("metadata").at("table_name") == "Rooms");

    CHECK(result.at(2).at("metadata").at("table_name") == "Bookings");

    CHECK(result.at(3).at("metadata").at("table_name") == "BookingModules");
}

TEST_CASE("rooms request contains image URL schema") {
    const auto result = database::build_batch_request(nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      2026);

    const auto& metadata = result.at(1).at("metadata");

    const auto& columns = metadata.at("columns");

    CHECK(std::find(columns.begin(), columns.end(), "imageUrl") != columns.end());

    CHECK_FALSE(metadata.contains("sql_before"));

    const auto sql_up = metadata.at("sql_up").get<std::string>();

    CHECK(sql_up.find("\"imageUrl\"") != std::string::npos);

    CHECK(metadata.at("write_mode") == "overwrite");
}

TEST_CASE("booking year cleanup is correctly expanded") {
    const auto result = database::build_batch_request(nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      2026);

    const auto sql_before = result.at(2).at("metadata").at("sql_before").get<std::string>();

    CHECK(sql_before.find("2026") != std::string::npos);

    CHECK(sql_before.find("2027") != std::string::npos);

    CHECK(sql_before.find("{0}") == std::string::npos);

    CHECK(sql_before.find("{1}") == std::string::npos);
}

TEST_CASE("database write modes remain correct") {
    const auto result = database::build_batch_request(nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      nlohmann::json::array(),
                                                      2026);

    CHECK(result.at(0).at("metadata").at("write_mode") == "overwrite");

    CHECK(result.at(1).at("metadata").at("write_mode") == "overwrite");

    CHECK(result.at(2).at("metadata").at("write_mode") == "append");

    CHECK(result.at(3).at("metadata").at("write_mode") == "append");
}