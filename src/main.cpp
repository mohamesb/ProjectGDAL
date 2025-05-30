#include "transform/transformer.h"
#include <iostream>
#include "gdal/GdalDataset.h"

int main() {
    geo::Transformer transformer;

    // Example 1: Reproject raster to WGS84
    transformer.ReprojectRaster("data/sample.tif", "data/reproject.tif", "EPSG:4326");

    // Example 2: Clip raster to a bounding box
    transformer.ClipRasterToBounds("data/sample.tif", "data/clipped.tif", -123.5, 37.0, -122.0, 38.0);

    try {
        GdalDataset dataset("../build/data/reproject.tif");
        auto size = dataset.getRasterSize();
        auto proj = dataset.getProjection();

        std::cout << "Raster size: " << size.first << "x" << size.second << std::endl;
        std::cout << "Projection: " << proj << std::endl;

        GdalDataset dataset1("../build/data/clipped.tif");
        auto size1 = dataset1.getRasterSize();
        auto proj1 = dataset1.getProjection();

        std::cout << "Raster size: " << size1.first << "x" << size1.second << std::endl;
        std::cout << "Projection: " << proj1 << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }



    return 0;
}




