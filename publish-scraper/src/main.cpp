#include "data/static_data.hpp"
#include "http/client.hpp"
#include "publish/client.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main() {
    try {
        const auto static_data =
            data::load_static_data("data");

        data::validate_static_data(static_data);

        std::cout
            << "Static data loaded successfully.\n"
            << "Buildings: "
            << static_data.buildings.size()
            << '\n'
            << "Rooms: "
            << static_data.rooms.size()
            << "\n\n";

        http::Client http_client;
        publish::Client publish_client{http_client};

        const auto view_options =
            publish_client.get_view_options();

        std::cout
            << "Publish API connection successful.\n\n";

        std::cout
            << "Time periods: "
            << view_options.time_periods.size()
            << '\n';

        std::cout
            << "Date periods: "
            << view_options.date_periods.size()
            << '\n';

        std::cout
            << "Weeks: "
            << view_options.weeks.size()
            << '\n';

        std::cout
            << "Days: "
            << view_options.days.size()
            << '\n';

        std::cout
            << "\nDate periods:\n";

        for (const auto& period : view_options.date_periods) {
            std::cout
                << "  "
                << period.description;

            if (period.is_default) {
                std::cout << " [default]";
            }

            std::cout << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}