#include "data/room_metadata.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace data {

void write_publish_ids(const std::filesystem::path& rooms_path,
                       const std::vector<rooms::PublishMatch>& matches) {
    using json = nlohmann::json;

    std::ifstream input{rooms_path};

    if (!input.is_open()) {
        throw std::runtime_error{"Could not open rooms file: " + rooms_path.string()};
    }

    json rooms_json;

    try {
        input >> rooms_json;
    } catch (const json::exception& error) {
        throw std::runtime_error{"Invalid rooms JSON: " + std::string{error.what()}};
    }

    input.close();

    if (!rooms_json.is_array()) {
        throw std::runtime_error{"rooms.json must contain a JSON array"};
    }

    std::unordered_map<std::string, std::string> publish_ids;

    for (const auto& match : matches) {
        publish_ids.emplace(match.room_id, match.publish_id);
    }

    std::size_t updated_count = 0;

    for (auto& room : rooms_json) {
        const auto room_id = room.at("id").get<std::string>();

        const auto match = publish_ids.find(room_id);

        if (match == publish_ids.end()) {
            continue;
        }

        room["publishId"] = match->second;

        ++updated_count;
    }

    if (updated_count != matches.size()) {
        throw std::runtime_error{"Only matched " + std::to_string(updated_count) + " rooms while " +
                                 std::to_string(matches.size()) + " Publish matches were expected"};
    }

    std::ofstream output{rooms_path};

    if (!output.is_open()) {
        throw std::runtime_error{"Could not write rooms file: " + rooms_path.string()};
    }

    output << rooms_json.dump(2) << '\n';
}

} // namespace data