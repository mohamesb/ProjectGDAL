cmake_minimum_required(VERSION 3.10)
project(TestSetup)

set(CMAKE_CXX_STANDARD 17)

find_package(GDAL REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(main main.cpp)

target_link_libraries(main
    GDAL::GDAL
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
