cmake_minimum_required(VERSION 3.12)
project(GeospatialPipeline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GDAL
find_package(GDAL REQUIRED)

# Set source files
set(SOURCES
    src/main.cpp
    src/gdal/GdalDataset.cpp
    src/gdal/GdalUtils.cpp
    src/transform/transformer.cpp
)

set(HEADERS
    src/gdal/GdalDataset.h
    src/gdal/GdalUtils.h
    src/transform/transformer.h
)

file(COPY ${CMAKE_SOURCE_DIR}/data/sample.tif DESTINATION ${CMAKE_BINARY_DIR}/data)


# Create executable
add_executable(main ${SOURCES} ${HEADERS})

# Include headers
target_include_directories(main PRIVATE ${GDAL_INCLUDE_DIR} src)

# Link GDAL
target_link_libraries(main PRIVATE ${GDAL_LIBRARY})

# Optional: add compile warnings
target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
