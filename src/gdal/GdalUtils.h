#pragma once
#include <string>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

namespace GdalUtils {
    std::string getCrsWktFromEPSG(int epsgCode);
    bool reprojectDataset(GDALDataset* inputDs, const std::string& outputPath, int targetEPSG);
}
