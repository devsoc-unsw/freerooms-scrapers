#include "http/client.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

using json =
    nlohmann::json;

std::string graphql_url() {
    const auto* value =
        std::getenv(
            "GRAPHQL_URL"
        );

    if (
        value == nullptr
        || *value == '\0'
    ) {
        return
            "http://localhost:8080/v1/graphql";
    }

    return value;
}

int scrape_year() {
    const auto* value =
        std::getenv(
            "YEAR"
        );

    if (
        value == nullptr
        || *value == '\0'
    ) {
        throw std::runtime_error{
            "Integration tests require YEAR"
        };
    }

    const std::string year_string{
        value
    };

    int year = 0;

    const auto result =
        std::from_chars(
            year_string.data(),
            year_string.data()
                + year_string.size(),
            year
        );

    if (
        result.ec != std::errc{}
        || result.ptr
            != year_string.data()
                + year_string.size()
    ) {
        throw std::runtime_error{
            "Invalid YEAR: "
            + year_string
        };
    }

    return year;
}

json graphql_request(
    const std::string& query,
    const json& variables =
        json::object()
) {
    http::Client client;

    const auto request =
        json{
            {
                "query",
                query
            },
            {
                "variables",
                variables
            }
        };

    const auto response =
        client.post_json(
            graphql_url(),
            request.dump()
        );

    if (
        response.status_code < 200
        || response.status_code >= 300
    ) {
        throw std::runtime_error{
            "GraphQL returned HTTP "
            + std::to_string(
                response.status_code
            )
            + ": "
            + response.body
        };
    }

    const auto body =
        json::parse(
            response.body
        );

    if (
        body.contains("errors")
        && !body.at("errors").empty()
    ) {
        throw std::runtime_error{
            "GraphQL returned errors: "
            + body.at("errors").dump()
        };
    }

    if (!body.contains("data")) {
        throw std::runtime_error{
            "GraphQL response did not contain data"
        };
    }

    return body.at("data");
}

std::string year_start(
    const int year
) {
    return
        std::to_string(year)
        + "-01-01T00:00:00Z";
}

}

TEST_CASE(
    "GraphQL exposes all buildings and rooms"
) {
    const auto data =
        graphql_request(
            R"(
                query {
                    buildings_aggregate {
                        aggregate {
                            count
                        }
                    }

                    rooms_aggregate {
                        aggregate {
                            count
                        }
                    }
                }
            )"
        );

    CHECK(
        data.at("buildings_aggregate")
            .at("aggregate")
            .at("count")
            .get<int>()
        == 44
    );

    CHECK(
        data.at("rooms_aggregate")
            .at("aggregate")
            .at("count")
            .get<int>()
        == 504
    );
}

TEST_CASE(
    "GraphQL exposes room image URLs"
) {
    const auto data =
        graphql_request(
            R"(
                query {
                    rooms_with_images:
                    rooms_aggregate(
                        where: {
                            imageUrl: {
                                _is_null: false
                            }
                        }
                    ) {
                        aggregate {
                            count
                        }
                    }

                    rooms_by_pk(
                        id: "K-B16-LG01"
                    ) {
                        id
                        name
                        imageUrl
                    }
                }
            )"
        );

    const auto image_count =
        data.at("rooms_with_images")
            .at("aggregate")
            .at("count")
            .get<int>();

    CHECK(
        image_count > 0
    );

    const auto& room =
        data.at("rooms_by_pk");

    REQUIRE_FALSE(
        room.is_null()
    );

    CHECK(
        room.at("id")
        == "K-B16-LG01"
    );

    REQUIRE_FALSE(
        room.at("imageUrl")
            .is_null()
    );

    const auto image_url =
        room.at("imageUrl")
            .get<std::string>();

    CHECK(
        image_url.starts_with(
            "https://www.learningenvironments.unsw.edu.au/"
        )
    );
}

TEST_CASE(
    "GraphQL exposes bookings and room relationships"
) {
    const auto year =
        scrape_year();

    const auto start =
        year_start(
            year
        );

    const auto end =
        year_start(
            year + 1
        );

    const auto data =
        graphql_request(
            R"(
                query BookingTest(
                    $start: timestamptz!,
                    $end: timestamptz!
                ) {
                    bookings_aggregate(
                        where: {
                            start: {
                                _gte: $start,
                                _lt: $end
                            }
                        }
                    ) {
                        aggregate {
                            count
                        }
                    }

                    bookings(
                        where: {
                            start: {
                                _gte: $start,
                                _lt: $end
                            }
                        }
                        order_by: {
                            start: asc
                        }
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

                        room {
                            id
                            name
                        }
                    }
                }
            )",
            {
                {
                    "start",
                    start
                },
                {
                    "end",
                    end
                }
            }
        );

    const auto count =
        data.at("bookings_aggregate")
            .at("aggregate")
            .at("count")
            .get<int>();

    CHECK(
        count > 0
    );

    const auto& bookings =
        data.at("bookings");

    REQUIRE(
        bookings.size() == 1
    );

    const auto& booking =
        bookings.at(0);

    CHECK_FALSE(
        booking.at("roomId")
            .get<std::string>()
            .empty()
    );

    CHECK_FALSE(
        booking.at("occurrenceId")
            .get<std::string>()
            .empty()
    );

    CHECK_FALSE(
        booking.at("eventId")
            .get<std::string>()
            .empty()
    );

    CHECK_FALSE(
        booking.at("name")
            .get<std::string>()
            .empty()
    );

    CHECK_FALSE(
        booking.at("eventType")
            .get<std::string>()
            .empty()
    );

    REQUIRE_FALSE(
        booking.at("room")
            .is_null()
    );

    CHECK(
        booking.at("room")
            .at("id")
        == booking.at("roomId")
    );
}

TEST_CASE(
    "GraphQL exposes booking module relationships in both directions"
) {
    const auto year =
        scrape_year();

    const auto start =
        year_start(
            year
        );

    const auto end =
        year_start(
            year + 1
        );

    const auto data =
        graphql_request(
            R"(
                query ModuleTest(
                    $start: timestamptz!,
                    $end: timestamptz!
                ) {
                    bookingmodules(
                        where: {
                            booking: {
                                start: {
                                    _gte: $start,
                                    _lt: $end
                                }
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
            {
                {
                    "start",
                    start
                },
                {
                    "end",
                    end
                }
            }
        );

    const auto& modules =
        data.at("bookingmodules");

    REQUIRE(
        modules.size() == 1
    );

    const auto& module =
        modules.at(0);

    CHECK_FALSE(
        module.at("code")
            .get<std::string>()
            .empty()
    );

    CHECK_FALSE(
        module.at("name")
            .get<std::string>()
            .empty()
    );

    REQUIRE_FALSE(
        module.at("booking")
            .is_null()
    );

    const auto& booking =
        module.at("booking");

    CHECK(
        booking.at("roomId")
        == module.at("roomId")
    );

    CHECK(
        booking.at("occurrenceId")
        == module.at("occurrenceId")
    );

    REQUIRE_FALSE(
        booking.at("room")
            .is_null()
    );

    CHECK(
        booking.at("room")
            .at("id")
        == module.at("roomId")
    );

    const auto& reverse_modules =
        booking.at("bookingmodules");

    REQUIRE_FALSE(
        reverse_modules.empty()
    );
}