#include "gdal/GdalDataset.h"
#include <iostream>

int main() {
    try {
        GdalDataset dataset("../data/sample.tif");
        auto size = dataset.getRasterSize();
        auto proj = dataset.getProjection();

        std::cout << "Raster size: " << size.first << "x" << size.second << std::endl;
        std::cout << "Projection: " << proj << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
