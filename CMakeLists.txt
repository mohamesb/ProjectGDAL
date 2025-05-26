#cmake_minimum_required(VERSION 3.10)
cmake_minimum_required(VERSION 3.14)
project(GeoPipeline CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project sources
set(SRC_FILES
    src/main.cpp
    src/configLoader.cpp
)

# Include directories
include_directories(include)

# Add executable
add_executable(geo_pipeline ${SRC_FILES})

# Optionally, you can treat warnings as errors for cleaner code
# target_compile_options(geo_pipeline PRIVATE -Wall -Wextra -Werror)

