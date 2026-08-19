#include "database/request.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string read_file(
    const std::string& path
) {
    std::ifstream file{path};

    if (!file) {
        throw std::runtime_error{
            "Failed to open file: "
            + path
        };
    }

    return std::string{
        std::istreambuf_iterator<char>{file},
        std::istreambuf_iterator<char>{}
    };
}

void replace_all(
    std::string& value,
    const std::string& target,
    const std::string& replacement
) {
    std::size_t position = 0;

    while (
        (
            position =
                value.find(
                    target,
                    position
                )
        )
        != std::string::npos
    ) {
        value.replace(
            position,
            target.size(),
            replacement
        );

        position +=
            replacement.size();
    }
}

std::string build_bookings_before_sql(
    const int year
) {
    auto sql =
        read_file(
            "sql/bookings/before.sql"
        );

    replace_all(
        sql,
        "{0}",
        std::to_string(year)
    );

    replace_all(
        sql,
        "{1}",
        std::to_string(year + 1)
    );

    return sql;
}

nlohmann::json build_bookings_request(
    nlohmann::json payload,
    const int year
) {
    auto metadata =
        nlohmann::json::object();

    metadata["table_name"] =
        "Bookings";

    metadata["columns"] =
        nlohmann::json::array(
            {
                "roomId",
                "occurrenceId",
                "eventId",
                "bookingType",
                "name",
                "rawName",
                "eventType",
                "start",
                "end",
                "plannedSize",
                "source",
                "lastModified"
            }
        );

    metadata["sql_up"] =
        read_file(
            "sql/bookings/up.sql"
        );

    metadata["sql_down"] =
        read_file(
            "sql/bookings/down.sql"
        );

    metadata["sql_before"] =
        build_bookings_before_sql(
            year
        );

    metadata["write_mode"] =
        "append";

    auto request =
        nlohmann::json::object();

    request["metadata"] =
        std::move(metadata);

    request["payload"] =
        std::move(payload);

    return request;
}

nlohmann::json build_modules_request(
    nlohmann::json payload
) {
    auto metadata =
        nlohmann::json::object();

    metadata["table_name"] =
        "BookingModules";

    metadata["columns"] =
        nlohmann::json::array(
            {
                "roomId",
                "occurrenceId",
                "moduleIndex",
                "code",
                "name",
                "term",
                "career"
            }
        );

    metadata["sql_up"] =
        read_file(
            "sql/booking_modules/up.sql"
        );

    metadata["sql_down"] =
        read_file(
            "sql/booking_modules/down.sql"
        );

    metadata["write_mode"] =
        "append";

    auto request =
        nlohmann::json::object();

    request["metadata"] =
        std::move(metadata);

    request["payload"] =
        std::move(payload);

    return request;
}

}

namespace database {

nlohmann::json build_batch_request(
    nlohmann::json booking_payload,
    nlohmann::json module_payload,
    const int year
) {
    if (!booking_payload.is_array()) {
        throw std::invalid_argument{
            "Booking payload must be an array"
        };
    }

    if (!module_payload.is_array()) {
        throw std::invalid_argument{
            "Module payload must be an array"
        };
    }

    auto result =
        nlohmann::json::array();

    result.push_back(
        build_bookings_request(
            std::move(
                booking_payload
            ),
            year
        )
    );

    result.push_back(
        build_modules_request(
            std::move(
                module_payload
            )
        )
    );

    return result;
}

}