#include "bookings/classification.hpp"
#include "bookings/filter.hpp"
#include "bookings/sort.hpp"
#include "types/booking.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

namespace {

model::Booking make_booking(std::string room_id, std::string occurrence_id, std::string start) {
    model::Booking booking;
    booking.room_id = std::move(room_id);
    booking.occurrence_id = std::move(occurrence_id);
    booking.start = std::move(start);
    return booking;
}

} // namespace

TEST_CASE("published normal booking is included") {
    model::Booking booking;

    booking.is_published = true;

    booking.is_deleted = false;

    booking.event_type = "Lecture";

    CHECK(bookings::should_include_booking(booking));
}

TEST_CASE("unpublished booking is excluded") {
    model::Booking booking;

    booking.is_published = false;

    booking.event_type = "Lecture";

    CHECK_FALSE(bookings::should_include_booking(booking));
}

TEST_CASE("deleted booking is excluded") {
    model::Booking booking;

    booking.is_published = true;

    booking.is_deleted = true;

    booking.event_type = "Lecture";

    CHECK_FALSE(bookings::should_include_booking(booking));
}

TEST_CASE("cancelled booking is excluded") {
    model::Booking booking;

    booking.is_published = true;

    booking.event_type = "BOOK.CANCELLED";

    CHECK_FALSE(bookings::should_include_booking(booking));
}

TEST_CASE("requested booking is excluded") {
    model::Booking booking;

    booking.is_published = true;

    booking.event_type = "BOOK.REQUESTED";

    CHECK_FALSE(bookings::should_include_booking(booking));
}

TEST_CASE("not used booking is excluded") {
    model::Booking booking;

    booking.is_published = true;

    booking.event_type = "*Not Used";

    CHECK_FALSE(bookings::should_include_booking(booking));
}

TEST_CASE("database booking type mappings remain stable") {
    CHECK(bookings::booking_type_database_value(model::BookingType::Lecture) == "LECTURE");

    CHECK(bookings::booking_type_database_value(model::BookingType::TutorialLaboratory) ==
          "TUTORIAL_LABORATORY");

    CHECK(bookings::booking_type_database_value(model::BookingType::Exam) == "EXAMS");

    CHECK(bookings::booking_type_database_value(model::BookingType::Society) == "SOCIETY");

    CHECK(bookings::booking_type_database_value(model::BookingType::Miscellaneous) == "MISC");
}

TEST_CASE("bookings are sorted by start then room then occurrence") {
    std::vector<model::Booking> values{
        make_booking("K-B-2", "2", "2026-01-02T10:00:00Z"),
        make_booking("K-B-2", "3", "2026-01-01T10:00:00Z"),
        make_booking("K-B-1", "9", "2026-01-01T10:00:00Z"),
        make_booking("K-B-2", "1", "2026-01-01T10:00:00Z"),
    };

    bookings::sort_bookings(values);

    REQUIRE(values.size() == 4);

    CHECK(values.at(0).room_id == "K-B-1");
    CHECK(values.at(1).occurrence_id == "1");
    CHECK(values.at(2).occurrence_id == "3");
    CHECK(values.at(3).start == "2026-01-02T10:00:00Z");
}
