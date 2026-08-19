#include "data/static_data.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    const std::filesystem::path data_directory =
        argc >= 2
            ? std::filesystem::path{argv[1]}
            : std::filesystem::path{"data"};

    try {
        const auto static_data =
            data::load_static_data(data_directory);

        data::validate_static_data(static_data);

        const auto missing_floor_count =
            std::count_if(
                static_data.rooms.begin(),
                static_data.rooms.end(),
                [](const model::Room& room) {
                    return !room.facilities.floor.has_value();
                }
            );

        const auto missing_seating_count =
            std::count_if(
                static_data.rooms.begin(),
                static_data.rooms.end(),
                [](const model::Room& room) {
                    return !room.facilities.seating.has_value();
                }
            );

        std::cout
            << "Static data loaded successfully.\n\n";

        std::cout
            << "Buildings: "
            << static_data.buildings.size()
            << '\n';

        std::cout
            << "Rooms: "
            << static_data.rooms.size()
            << '\n';

        std::cout
            << "Rooms without floor data: "
            << missing_floor_count
            << '\n';

        std::cout
            << "Rooms without seating data: "
            << missing_seating_count
            << '\n';

        if (!static_data.rooms.empty()) {
            const auto& room = static_data.rooms.front();

            std::cout
                << "\nFirst room:\n"
                << "  ID: "
                << room.id
                << '\n'
                << "  Name: "
                << room.name
                << '\n'
                << "  Capacity: "
                << room.capacity
                << '\n'
                << "  Building: "
                << room.building_id
                << '\n';
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
