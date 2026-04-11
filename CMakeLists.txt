cmake_minimum_required(VERSION 3.16)
project(airports LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ---------------------------------------------------------------------------
#  Основное приложение
# ---------------------------------------------------------------------------
add_executable(airports
    src/main.cpp
    src/AirportManager.cpp
    src/MainWindow.cpp
)

target_include_directories(airports PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(airports PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

add_custom_command(TARGET airports POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/airports.csv
        $<TARGET_FILE_DIR:airports>/airports.csv
)

# ---------------------------------------------------------------------------
#  Тесты (Google Test)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(airports_tests
    tests/AirportManagerTest.cpp
    src/AirportManager.cpp
)

target_include_directories(airports_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(airports_tests PRIVATE Qt6::Core gtest_main)

add_custom_command(TARGET airports_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/res/airports.csv
        $<TARGET_FILE_DIR:airports_tests>/airports.csv
)

include(GoogleTest)
gtest_discover_tests(airports_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
