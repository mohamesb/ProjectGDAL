cmake_minimum_required(VERSION 3.16)
project(GeoProcessingPipeline VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GDAL REQUIRED gdal)

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(${GDAL_INCLUDE_DIRS})
include_directories(src)

# Add executable
add_executable(geo_pipeline
    src/main.cpp
    src/config/config.cpp
    src/gdal/GdalDataset.cpp
    src/transform/transformer.cpp
    src/pipeline/pipeline.cpp
)

# Link libraries
target_link_libraries(geo_pipeline 
    ${GDAL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Compiler flags
target_compile_options(geo_pipeline PRIVATE ${GDAL_CFLAGS_OTHER})

# Set output directory
set_target_properties(geo_pipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS geo_pipeline DESTINATION bin)
