#include <iostream>
#include <gdal.h>
#include <gdal_priv.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

int main() {
    // spdlog test
    spdlog::info("Starting test program...");

    // JSON test
    nlohmann::json j;
    j["name"] = "Geospatial Pipeline";
    j["version"] = 1.0;
    spdlog::info("JSON object: {}", j.dump());

    // GDAL test
    GDALAllRegister();
    GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen("test.tif", GA_ReadOnly));

    if (dataset) {
        spdlog::info("Opened test.tif successfully with GDAL.");
        GDALClose(dataset);
    } else {
        spdlog::warn("Failed to open test.tif (expected if file not present).");
    }

    spdlog::info("Environment setup verified.");
    return 0;
}
