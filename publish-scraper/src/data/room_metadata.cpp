#include "data/room_metadata.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace data {

PublishIdWriteResult write_publish_ids(const std::filesystem::path& rooms_path,
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

    std::size_t matched_count = 0;
    std::size_t changed_count = 0;

    for (auto& room : rooms_json) {
        const auto room_id = room.at("id").get<std::string>();

        const auto match = publish_ids.find(room_id);

        if (match == publish_ids.end()) {
            continue;
        }

        ++matched_count;

        if (!room.contains("publishId") || room.at("publishId").is_null() ||
            room.at("publishId").get<std::string>() != match->second) {
            room["publishId"] = match->second;
            ++changed_count;
        }
    }

    if (matched_count != matches.size()) {
        throw std::runtime_error{"Only matched " + std::to_string(matched_count) + " rooms while " +
                                 std::to_string(matches.size()) + " Publish matches were expected"};
    }

    std::unordered_set<std::string> final_publish_ids;

    for (const auto& room : rooms_json) {
        if (!room.contains("publishId") || room.at("publishId").is_null()) {
            continue;
        }

        const auto publish_id = room.at("publishId").get<std::string>();

        if (publish_id.empty()) {
            throw std::runtime_error{"Room " + room.at("id").get<std::string>() +
                                     " has an empty Publish ID"};
        }

        if (!final_publish_ids.insert(publish_id).second) {
            throw std::runtime_error{"Room mapping update would create duplicate Publish ID: " +
                                     publish_id};
        }
    }

    auto temporary_path = rooms_path;
    temporary_path += ".tmp";

    std::ofstream output{temporary_path};

    if (!output.is_open()) {
        throw std::runtime_error{"Could not write temporary rooms file: " +
                                 temporary_path.string()};
    }

    output << rooms_json.dump(2) << '\n';
    output.close();

    if (!output) {
        std::filesystem::remove(temporary_path);
        throw std::runtime_error{"Failed while writing rooms file: " + rooms_path.string()};
    }

    std::error_code rename_error;
    std::filesystem::rename(temporary_path, rooms_path, rename_error);

    if (rename_error) {
        std::filesystem::remove(temporary_path);
        throw std::runtime_error{"Could not replace rooms file: " + rename_error.message()};
    }

    return PublishIdWriteResult{
        .matched_rooms = matched_count,
        .changed_publish_ids = changed_count,
    };
}

} // namespace data