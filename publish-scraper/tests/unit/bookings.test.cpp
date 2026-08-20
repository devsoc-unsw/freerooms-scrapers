#include "bookings/classification.hpp"
#include "bookings/filter.hpp"
#include "types/booking.hpp"

#include <catch2/catch_test_macros.hpp>

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