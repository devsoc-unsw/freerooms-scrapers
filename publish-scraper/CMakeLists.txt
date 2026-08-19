cmake_minimum_required(VERSION 3.20)

project(
    freerooms_publish_scraper
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(freerooms_types INTERFACE)

target_include_directories(
    freerooms_types
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(
    publish_scraper
    src/main.cpp
)

target_link_libraries(
    publish_scraper
    PRIVATE
        freerooms_types
)

set_target_properties(
    publish_scraper
    PROPERTIES
        OUTPUT_NAME "publish-scraper"
)

if(MSVC)
    target_compile_options(
        publish_scraper
        PRIVATE
            /W4
    )
else()
    target_compile_options(
        publish_scraper
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
    )
endif()
