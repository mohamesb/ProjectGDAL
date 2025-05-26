#include "GdalUtils.h"
#include <stdexcept>

std::string GdalUtils::getCrsWktFromEPSG(int epsgCode) {
    OGRSpatialReference srs;
    if (srs.importFromEPSG(epsgCode) != OGRERR_NONE) {
        throw std::runtime_error("Invalid EPSG code.");
    }

    char* wkt = nullptr;
    srs.exportToWkt(&wkt);
    std::string result(wkt);
    CPLFree(wkt);
    return result;
}

bool GdalUtils::reprojectDataset(GDALDataset* inputDs, const std::string& outputPath, int targetEPSG) {
    OGRSpatialReference targetSRS;
    targetSRS.importFromEPSG(targetEPSG);

    char* pszSRS_WKT = nullptr;
    targetSRS.exportToWkt(&pszSRS_WKT);

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) return false;

    GDALDataset* outDs = driver->CreateCopy(outputPath.c_str(), inputDs, FALSE, NULL, NULL, NULL);
    if (!outDs) return false;

    outDs->SetProjection(pszSRS_WKT);
    CPLFree(pszSRS_WKT);
    GDALClose(outDs);
    return true;
}
