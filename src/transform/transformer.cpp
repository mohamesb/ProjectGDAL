#include "transformer.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "gdalwarper.h"       // GDALAutoCreateWarpedVRT, GRA_*
#include "gdal_utils.h"       // GDALWarp, GDALWarpAppOptions*
#include "cpl_conv.h"         // CPLFree
#include <iostream>
#include <vector>

namespace geo {

Transformer::Transformer() {
    Init();
}

bool Transformer::Init() {
    GDALAllRegister();
    return true;
}

bool Transformer::Cleanup() {
    GDALDestroyDriverManager();
    return true;
}

bool Transformer::ReprojectRaster(const std::string& inputPath,
    const std::string& outputPath,
    const std::string& targetEPSG)
{
    GDALAllRegister();

    // Open source dataset
    GDALDatasetH hSrcDS = GDALOpen(inputPath.c_str(), GA_ReadOnly);
    if (!hSrcDS) {
    std::cerr << "❌ Cannot open input file: " << inputPath << "\n";
    return false;
    }

    // Get source SRS
    const char* pszSrcWKT = GDALGetProjectionRef(hSrcDS);
    if (!pszSrcWKT || std::string(pszSrcWKT).empty()) {
    std::cerr << "❌ Source projection is missing.\n";
    GDALClose(hSrcDS);
    return false;
    }

    // Setup target SRS
    OGRSpatialReference oTargetSRS;
    if (oTargetSRS.SetFromUserInput(targetEPSG.c_str()) != OGRERR_NONE) {
    std::cerr << "❌ Invalid target EPSG string: " << targetEPSG << "\n";
    GDALClose(hSrcDS);
    return false;
    }

    char* pszDstWKT = nullptr;
    if (oTargetSRS.exportToWkt(&pszDstWKT) != OGRERR_NONE) {
    std::cerr << "❌ Failed to export target SRS to WKT.\n";
    GDALClose(hSrcDS);
    return false;
    }

    // Create warped VRT (in-memory)
    GDALDatasetH hWarpedVRT = GDALAutoCreateWarpedVRT(
    hSrcDS,
    pszSrcWKT,
    pszDstWKT,
    GRA_NearestNeighbour,
    0.0,
    nullptr
    );

    CPLFree(pszDstWKT);  // Free the WKT string

    if (!hWarpedVRT) {
    std::cerr << "❌ Failed to create warped VRT.\n";
    GDALClose(hSrcDS);
    return false;
}

// Get GTiff driver
GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
if (!poDriver) {
std::cerr << "❌ GTiff driver not available.\n";
GDALClose(hWarpedVRT);
GDALClose(hSrcDS);
return false;
}

// Write final output
GDALDatasetH hDstDS = GDALCreateCopy(
poDriver,
outputPath.c_str(),
hWarpedVRT,
FALSE,     // strict
nullptr,   // options
nullptr, nullptr
);

if (!hDstDS) {
std::cerr << "❌ Failed to write output raster: " << outputPath << "\n";
GDALClose(hWarpedVRT);
GDALClose(hSrcDS);
return false;
}

// Cleanup
GDALClose(hDstDS);
GDALClose(hWarpedVRT);
GDALClose(hSrcDS);

std::cout << "✅ Raster reprojected to: " << outputPath << "\n";
return true;
}


bool Transformer::ClipRasterToBounds(const std::string& inputPath,
    const std::string& outputPath,
    double minX, double minY, double maxX, double maxY) {
    GDALDatasetH hSrcDS = GDALOpen(inputPath.c_str(), GA_ReadOnly);
    if (!hSrcDS) {
        std::cerr << "❌ Failed to open input raster: " << inputPath << "\n";
        return false;
    }

    // Build warp options for clipping
    char** warpOptions = nullptr;

    std::string cutline = std::to_string(minX) + " " + std::to_string(minY) + " " +
                        std::to_string(maxX) + " " + std::to_string(minY) + " " +
                        std::to_string(maxX) + " " + std::to_string(maxY) + " " +
                        std::to_string(minX) + " " + std::to_string(maxY) + " " +
                        std::to_string(minX) + " " + std::to_string(minY);

    warpOptions = CSLSetNameValue(warpOptions, "CUTLINE", cutline.c_str());
    warpOptions = CSLSetNameValue(warpOptions, "CROP_TO_CUTLINE", "YES");

    GDALWarpAppOptions* psWarpOptions = GDALWarpAppOptionsNew(warpOptions, nullptr);
    if (!psWarpOptions) {
        std::cerr << "❌ Failed to create GDAL warp options.\n";
        GDALClose(hSrcDS);
        return false;
    }

    int usageError = FALSE;

    GDALDatasetH hDstDS = GDALWarp(
        outputPath.c_str(),
        nullptr,
        1,
        &hSrcDS,
        psWarpOptions,
        &usageError
    );

    GDALWarpAppOptionsFree(psWarpOptions);
    CSLDestroy(warpOptions);
    GDALClose(hSrcDS);

    if (usageError || !hDstDS) {
        std::cerr << "❌ GDALWarp failed to clip raster.\n";
        return false;
    }

    GDALClose(hDstDS);


    std::cout << "✅ Clipped raster written to: " << outputPath << "\n";
    return true;
}

}  // namespace geo

