#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <string>
#include <memory>
#include "gdal_priv.h"
#include "ogr_spatialref.h"

namespace geo {

class Transformer {
public:
    Transformer();

    bool ReprojectRaster(const std::string& inputPath,
                         const std::string& outputPath,
                         const std::string& targetEPSG);

    bool ClipRasterToBounds(const std::string& inputPath,
                            const std::string& outputPath,
                            double minX, double minY, double maxX, double maxY);

private:
    bool Init();
    bool Cleanup();
};

}  // namespace geo

#endif // TRANSFORMER_H
