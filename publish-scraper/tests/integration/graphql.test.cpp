#include "config/exclusions.hpp"
#include "data/static_data.hpp"
#include "http/client.hpp"
#include "rooms/exclusions.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using json = nlohmann::json;

std::string graphql_url() {
    const auto* value = std::getenv("GRAPHQL_URL");

    if (value == nullptr || *value == '\0') {
        return "http://localhost:8080/v1/graphql";
    }

    return value;
}

int current_sydney_year() {
    using namespace std::chrono;

    const auto sydney_time = system_clock::now() + hours{11};
    const auto time = system_clock::to_time_t(sydney_time);
    const auto* calendar_time = std::gmtime(&time);

    if (calendar_time == nullptr) {
        throw std::runtime_error{"Could not determine current year"};
    }

    return calendar_time->tm_year + 1900;
}

int scrape_year() {
    const auto* value = std::getenv("YEAR");

    if (value == nullptr || *value == '\0') {
        return current_sydney_year();
    }

    const std::string year_string{value};

    int year = 0;

    const auto result =
        std::from_chars(year_string.data(), year_string.data() + year_string.size(), year);

    if (result.ec != std::errc{} || result.ptr != year_string.data() + year_string.size()) {
        throw std::runtime_error{"Invalid YEAR: " + year_string};
    }

    return year;
}

json graphql_request(const std::string& query, const json& variables = json::object()) {
    http::Client client;

    const auto request = json{{"query", query}, {"variables", variables}};

    const auto response = client.post_json(graphql_url(), request.dump());

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error{"GraphQL returned HTTP " + std::to_string(response.status_code) +
                                 ": " + response.body};
    }

    const auto body = json::parse(response.body);

    if (body.contains("errors") && !body.at("errors").empty()) {
        throw std::runtime_error{"GraphQL returned errors: " + body.at("errors").dump()};
    }

    if (!body.contains("data")) {
        throw std::runtime_error{"GraphQL response did not contain data"};
    }

    return body.at("data");
}

std::string year_start(const int year) {
    return std::to_string(year) + "-01-01T00:00:00Z";
}

data::StaticData expected_static_data() {
    auto result = data::load_static_data("data");
    const auto exclusions = config::load_exclusions("config/exclusions.json");

    rooms::filter_excluded_static_rooms(result.rooms, exclusions);

    return result;
}

template <typename Value, typename Compare>
void check_ascending(const Value& values, Compare compare, const std::string_view description) {
    for (std::size_t index = 1; index < values.size(); ++index) {
        CAPTURE(description, index);
        CHECK_FALSE(compare(values.at(index), values.at(index - 1)));
    }
}

std::string floor_value(const std::optional<model::FloorType>& floor) {
    if (!floor.has_value()) {
        return {};
    }

    switch (*floor) {
    case model::FloorType::Flat:
        return "Flat";
    case model::FloorType::Tiered:
        return "Tiered";
    case model::FloorType::Other:
        return "Other";
    }

    throw std::runtime_error{"Unknown floor type"};
}

std::string seating_value(const std::optional<model::SeatingType>& seating) {
    if (!seating.has_value()) {
        return {};
    }

    switch (*seating) {
    case model::SeatingType::Movable:
        return "Movable";
    case model::SeatingType::Fixed:
        return "Fixed";
    }

    throw std::runtime_error{"Unknown seating type"};
}

json first_booking_in_year() {
    const auto year = scrape_year();

    const auto data = graphql_request(
        R"(
            query FirstBooking($start: timestamptz!, $end: timestamptz!) {
                bookings(
                    where: {
                        start: {_lt: $end}
                        end: {_gt: $start}
                    }
                    order_by: [{start: asc}, {roomId: asc}]
                    limit: 1
                ) {
                    roomId
                    start
                    end
                }
            }
        )",
        {{"start", year_start(year)}, {"end", year_start(year + 1)}});

    const auto& bookings = data.at("bookings");

    REQUIRE_FALSE(bookings.empty());

    return bookings.at(0);
}

} // namespace

TEST_CASE("GraphQL returns every active building and room in the expected order") {
    const auto expected = expected_static_data();

    std::set<std::string> expected_building_ids;
    std::set<std::string> expected_room_ids;

    for (const auto& room : expected.rooms) {
        expected_building_ids.insert(room.building_id);
        expected_room_ids.insert(room.id);
    }

    const auto data = graphql_request(
        R"(
            query BuildingAndRooms {
                buildings(order_by: {name: asc}) {
                    id
                    name
                    lat
                    long
                    aliases
                    rooms(order_by: {id: asc}) {
                        id
                        name
                        abbr
                        school
                        usage
                        capacity
                    }
                }
            }
        )");

    const auto& buildings = data.at("buildings");

    CHECK(buildings.size() == expected_building_ids.size());

    check_ascending(
        buildings,
        [](const json& left, const json& right) {
            return left.at("name").get<std::string>() < right.at("name").get<std::string>();
        },
        "buildings by name");

    std::set<std::string> returned_room_ids;

    for (const auto& building : buildings) {
        const auto building_id = building.at("id").get<std::string>();

        CHECK(expected_building_ids.contains(building_id));
        CHECK(building.at("name").is_string());
        CHECK(building.at("lat").is_number());
        CHECK(building.at("long").is_number());
        CHECK(building.at("aliases").is_array());

        const auto& building_rooms = building.at("rooms");

        check_ascending(
            building_rooms,
            [](const json& left, const json& right) {
                return left.at("id").get<std::string>() < right.at("id").get<std::string>();
            },
            "rooms by id");

        for (const auto& room : building_rooms) {
            const auto room_id = room.at("id").get<std::string>();

            CHECK(room_id.starts_with(building_id + "-"));
            CHECK(room.at("name").is_string());
            CHECK(room.at("abbr").is_string());
            CHECK(room.at("school").is_string());
            CHECK(room.at("usage").is_string());
            CHECK(room.at("capacity").is_number_integer());

            returned_room_ids.insert(room_id);
        }
    }

    CHECK(returned_room_ids == expected_room_ids);
}

TEST_CASE("GraphQL booking range queries return overlapping bookings sorted by start") {
    const auto year = scrape_year();
    const auto start = year_start(year);
    const auto end = year_start(year + 1);

    const auto data = graphql_request(
        R"(
            query BookingsInRange($start: timestamptz!, $end: timestamptz!) {
                rooms {
                    id
                    name
                    bookings(
                        where: {
                            start: {_lte: $end}
                            end: {_gte: $start}
                        }
                        order_by: {start: asc}
                    ) {
                        name
                        bookingType
                        start
                        end
                    }
                }
            }
        )",
        {{"start", start}, {"end", end}});

    std::size_t booking_count = 0;

    for (const auto& room : data.at("rooms")) {
        const auto& bookings = room.at("bookings");

        check_ascending(
            bookings,
            [](const json& left, const json& right) {
                return left.at("start").get<std::string>() < right.at("start").get<std::string>();
            },
            "room bookings by start");

        for (const auto& booking : bookings) {
            ++booking_count;

            CHECK(booking.at("name").is_string());
            CHECK_FALSE(booking.at("name").get<std::string>().empty());
            CHECK(booking.at("bookingType").is_string());
            CHECK(booking.at("start").is_string());
            CHECK(booking.at("end").is_string());
        }
    }

    CHECK(booking_count > 0);
}

TEST_CASE("GraphQL booking range queries are inclusive at start and end boundaries") {
    const auto target = first_booking_in_year();

    for (const auto& boundary :
         {target.at("start").get<std::string>(), target.at("end").get<std::string>()}) {
        const auto data = graphql_request(
            R"(
                query BoundaryTest($start: timestamptz!, $end: timestamptz!) {
                    rooms {
                        id
                        bookings(
                            where: {
                                start: {_lte: $end}
                                end: {_gte: $start}
                            }
                            order_by: {start: asc}
                        ) {
                            start
                            end
                        }
                    }
                }
            )",
            {{"start", boundary}, {"end", boundary}});

        const auto room_id = target.at("roomId").get<std::string>();

        const json* matching_room = nullptr;

        for (const auto& candidate : data.at("rooms")) {
            if (candidate.at("id") == room_id) {
                matching_room = &candidate;
                break;
            }
        }

        REQUIRE(matching_room != nullptr);

        bool found = false;

        for (const auto& booking : matching_room->at("bookings")) {
            if (booking.at("start") == target.at("start") &&
                booking.at("end") == target.at("end")) {
                found = true;
                break;
            }
        }

        CHECK(found);
    }
}

TEST_CASE("GraphQL booking range queries return no stale historical bookings") {
    const auto year = scrape_year() - 20;

    const auto data = graphql_request(
        R"(
            query HistoricalRange($start: timestamptz!, $end: timestamptz!) {
                rooms {
                    id
                    bookings(
                        where: {
                            start: {_lte: $end}
                            end: {_gte: $start}
                        }
                    ) {
                        occurrenceId
                    }
                }
            }
        )",
        {{"start", year_start(year)}, {"end", std::to_string(year) + "-01-02T00:00:00Z"}});

    REQUIRE_FALSE(data.at("rooms").empty());

    for (const auto& room : data.at("rooms")) {
        CHECK(room.at("bookings").empty());
    }
}

TEST_CASE("GraphQL returns one room's bookings sorted by start") {
    const auto target = first_booking_in_year();
    const auto room_id = target.at("roomId").get<std::string>();

    const auto data = graphql_request(
        R"(
            query BookingsForRoom($roomId: String!) {
                rooms_by_pk(id: $roomId) {
                    id
                    name
                    bookings(order_by: {start: asc}) {
                        name
                        bookingType
                        start
                        end
                    }
                }
            }
        )",
        {{"roomId", room_id}});

    const auto& room = data.at("rooms_by_pk");

    REQUIRE_FALSE(room.is_null());
    CHECK(room.at("id") == room_id);
    CHECK(room.at("name").is_string());
    REQUIRE_FALSE(room.at("bookings").empty());

    check_ascending(
        room.at("bookings"),
        [](const json& left, const json& right) {
            return left.at("start").get<std::string>() < right.at("start").get<std::string>();
        },
        "bookings for one room");
}

TEST_CASE("GraphQL returns null for an unknown room id") {
    const auto data = graphql_request(
        R"(
            query UnknownRoom($roomId: String!) {
                rooms_by_pk(id: $roomId) {
                    id
                }
            }
        )",
        {{"roomId", "CI-NOT-A-REAL-ROOM"}});

    CHECK(data.at("rooms_by_pk").is_null());
}

TEST_CASE("GraphQL exposes the room utility fields inserted by the scraper") {
    const auto expected_data = expected_static_data();

    REQUIRE_FALSE(expected_data.rooms.empty());

    const auto& expected = expected_data.rooms.front();

    const auto data = graphql_request(
        R"(
            query RoomUtilities($roomId: String!) {
                rooms_by_pk(id: $roomId) {
                    id
                    name
                    floor
                    seating
                    microphone
                    accessibility
                    audiovisual
                    infotechnology
                    writingMedia
                    service
                }
            }
        )",
        {{"roomId", expected.id}});

    const auto& room = data.at("rooms_by_pk");

    REQUIRE_FALSE(room.is_null());
    CHECK(room.at("id") == expected.id);
    CHECK(room.at("name") == expected.name);

    if (expected.facilities.floor.has_value()) {
        CHECK(room.at("floor") == floor_value(expected.facilities.floor));
    } else {
        CHECK(room.at("floor").is_null());
    }

    if (expected.facilities.seating.has_value()) {
        CHECK(room.at("seating") == seating_value(expected.facilities.seating));
    } else {
        CHECK(room.at("seating").is_null());
    }

    CHECK(room.at("microphone").get<std::vector<std::string>>() == expected.facilities.microphone);
    CHECK(room.at("accessibility").get<std::vector<std::string>>() ==
          expected.facilities.accessibility);
    CHECK(room.at("audiovisual").get<std::vector<std::string>>() ==
          expected.facilities.audiovisual);
    CHECK(room.at("infotechnology").get<std::vector<std::string>>() ==
          expected.facilities.information_technology);
    CHECK(room.at("writingMedia").get<std::vector<std::string>>() ==
          expected.facilities.writing_media);
    CHECK(room.at("service").get<std::vector<std::string>>() == expected.facilities.services);
}

TEST_CASE("GraphQL room utility query returns null for an unknown room id") {
    const auto data = graphql_request(
        R"(
            query UnknownRoomUtilities($roomId: String!) {
                rooms_by_pk(id: $roomId) {
                    id
                    floor
                }
            }
        )",
        {{"roomId", "CI-NOT-A-REAL-ROOM"}});

    CHECK(data.at("rooms_by_pk").is_null());
}

TEST_CASE("GraphQL exposes room image URLs") {
    const auto data = graphql_request(
        R"(
            query RoomImages {
                rooms_with_images: rooms_aggregate(
                    where: {
                        imageUrl: {_is_null: false}
                    }
                ) {
                    aggregate {
                        count
                    }
                }

                rooms(
                    where: {
                        imageUrl: {_is_null: false}
                    }
                    order_by: {id: asc}
                    limit: 1
                ) {
                    id
                    name
                    imageUrl
                }
            }
        )");

    const auto image_count = data.at("rooms_with_images").at("aggregate").at("count").get<int>();

    CHECK(image_count > 0);

    const auto& returned_rooms = data.at("rooms");

    REQUIRE(returned_rooms.size() == 1);

    const auto& room = returned_rooms.at(0);

    REQUIRE_FALSE(room.at("imageUrl").is_null());

    CHECK(room.at("imageUrl")
              .get<std::string>()
              .starts_with("https://www.learningenvironments.unsw.edu.au/"));
}

TEST_CASE("GraphQL exposes detailed booking fields and the room relationship") {
    const auto year = scrape_year();

    const auto data = graphql_request(
        R"(
            query DetailedBooking($start: timestamptz!, $end: timestamptz!) {
                bookings(
                    where: {
                        start: {_gte: $start, _lt: $end}
                    }
                    order_by: [{start: asc}, {roomId: asc}]
                    limit: 1
                ) {
                    roomId
                    occurrenceId
                    eventId
                    bookingType
                    name
                    rawName
                    eventType
                    start
                    end
                    plannedSize
                    source
                    lastModified
                    room {
                        id
                        name
                    }
                }
            }
        )",
        {{"start", year_start(year)}, {"end", year_start(year + 1)}});

    const auto& bookings = data.at("bookings");

    REQUIRE(bookings.size() == 1);

    const auto& booking = bookings.at(0);

    CHECK_FALSE(booking.at("roomId").get<std::string>().empty());
    CHECK_FALSE(booking.at("occurrenceId").get<std::string>().empty());
    CHECK_FALSE(booking.at("eventId").get<std::string>().empty());
    CHECK_FALSE(booking.at("name").get<std::string>().empty());
    CHECK_FALSE(booking.at("rawName").get<std::string>().empty());
    CHECK_FALSE(booking.at("eventType").get<std::string>().empty());
    REQUIRE_FALSE(booking.at("room").is_null());
    CHECK(booking.at("room").at("id") == booking.at("roomId"));
}

TEST_CASE("GraphQL exposes booking module relationships in both directions") {
    const auto year = scrape_year();

    const auto data = graphql_request(
        R"(
            query ModuleTest($start: timestamptz!, $end: timestamptz!) {
                bookingmodules(
                    where: {
                        booking: {
                            start: {_gte: $start, _lt: $end}
                        }
                    }
                    limit: 1
                ) {
                    roomId
                    occurrenceId
                    code
                    name
                    term
                    career
                    booking {
                        roomId
                        occurrenceId
                        name
                        room {
                            id
                            name
                        }
                        bookingmodules {
                            code
                            name
                        }
                    }
                }
            }
        )",
        {{"start", year_start(year)}, {"end", year_start(year + 1)}});

    const auto& modules = data.at("bookingmodules");

    REQUIRE(modules.size() == 1);

    const auto& module = modules.at(0);

    CHECK_FALSE(module.at("code").get<std::string>().empty());
    CHECK_FALSE(module.at("name").get<std::string>().empty());
    REQUIRE_FALSE(module.at("booking").is_null());

    const auto& booking = module.at("booking");

    CHECK(booking.at("roomId") == module.at("roomId"));
    CHECK(booking.at("occurrenceId") == module.at("occurrenceId"));
    REQUIRE_FALSE(booking.at("room").is_null());
    CHECK(booking.at("room").at("id") == module.at("roomId"));
    REQUIRE_FALSE(booking.at("bookingmodules").empty());
}
