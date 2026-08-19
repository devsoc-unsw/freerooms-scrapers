cmake_minimum_required(VERSION 3.20)

project(
    publish_scraper
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

find_package(nlohmann_json 3.11.3 QUIET)
find_package(CURL REQUIRED)

if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(
    scraper_core
    src/data/static_data.cpp
    src/http/client.cpp
    src/publish/client.cpp
    src/publish/json.cpp
)

target_include_directories(
    scraper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    scraper_core
    PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

add_executable(
    publish_scraper
    src/main.cpp
)

target_link_libraries(
    publish_scraper
    PRIVATE
        scraper_core
)

set_target_properties(
    publish_scraper
    PROPERTIES
        OUTPUT_NAME "publish-scraper"
)

if(MSVC)
    target_compile_options(
        scraper_core
        PRIVATE /W4
    )

    target_compile_options(
        publish_scraper
        PRIVATE /W4
    )
else()
    target_compile_options(
        scraper_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
    )

    target_compile_options(
        publish_scraper
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
    )
endif()