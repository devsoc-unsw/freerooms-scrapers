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
include(CTest)

find_package(
    nlohmann_json
    3.11.3
    QUIET
)

find_package(
    CURL
    REQUIRED
)

find_package(
    Threads
    REQUIRED
)

if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY
            https://github.com/nlohmann/json.git
        GIT_TAG
            v3.11.3
        GIT_SHALLOW
            TRUE
    )

    FetchContent_MakeAvailable(
        nlohmann_json
    )
endif()

add_library(
    scraper_core
    src/data/static_data.cpp
    src/http/client.cpp
    src/publish/client.cpp
    src/publish/json.cpp
    src/publish/request.cpp
    src/rooms/publish_mapping.cpp
    src/rooms/image_url.cpp
    src/data/room_metadata.cpp
    src/config/exclusions.cpp
    src/rooms/exclusions.cpp
    src/rooms/room_id.cpp
    src/bookings/transform.cpp
    src/bookings/classification.cpp
    src/bookings/modules.cpp
    src/bookings/name_parser.cpp
    src/bookings/filter.cpp
    src/bookings/json.cpp
    src/database/request.cpp
    src/database/static_json.cpp
    src/database/client.cpp
)

target_include_directories(
    scraper_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    scraper_core
    PRIVATE
        CURL::libcurl
        Threads::Threads
    PUBLIC
        nlohmann_json::nlohmann_json
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
        OUTPUT_NAME
            "publish-scraper"
)

function(enable_project_warnings target)
    if(MSVC)
        target_compile_options(
            ${target}
            PRIVATE
                /W4
        )
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wshadow
        )
    endif()
endfunction()

enable_project_warnings(
    scraper_core
)

enable_project_warnings(
    publish_scraper
)

if(BUILD_TESTING)
    find_package(
        Catch2
        3
        QUIET
    )

    if(NOT Catch2_FOUND)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY
                https://github.com/catchorg/Catch2.git
            GIT_TAG
                v3.8.1
            GIT_SHALLOW
                TRUE
        )

        FetchContent_MakeAvailable(
            Catch2
        )

        list(
            APPEND
            CMAKE_MODULE_PATH
            ${catch2_SOURCE_DIR}/extras
        )
    endif()

    include(Catch)

    add_executable(
        publish_scraper_unit_tests
        tests/unit/static_data.test.cpp
        tests/unit/image_url.test.cpp
        tests/unit/static_json.test.cpp
        tests/unit/bookings.test.cpp
        tests/unit/database_request.test.cpp
        tests/unit/publish_retry.test.cpp
    )

    target_link_libraries(
        publish_scraper_unit_tests
        PRIVATE
            scraper_core
            Catch2::Catch2WithMain
    )

    enable_project_warnings(
        publish_scraper_unit_tests
    )

    catch_discover_tests(
        publish_scraper_unit_tests
        TEST_PREFIX
            "unit: "
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES
            LABELS unit
    )

    add_executable(
        publish_scraper_integration_tests
        tests/integration/graphql.test.cpp
    )

    target_link_libraries(
        publish_scraper_integration_tests
        PRIVATE
            scraper_core
            Catch2::Catch2WithMain
    )

    enable_project_warnings(
        publish_scraper_integration_tests
    )

    catch_discover_tests(
        publish_scraper_integration_tests
        TEST_PREFIX
            "integration: "
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES
            LABELS integration
    )
endif()