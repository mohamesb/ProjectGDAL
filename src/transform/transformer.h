#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <string>

namespace geo {

class Transformer {
public:
    Transformer();

    // Reproject raster from inputPath to targetEPSG (e.g., "EPSG:4326")
    bool ReprojectRaster(const std::string& inputPath,
                         const std::string& outputPath,
                         const std::string& targetEPSG);

    // Clip raster to bounding box (in target CRS)
    bool ClipRasterToBounds(const std::string& inputPath,
                            const std::string& outputPath,
                            double minX, double minY, double maxX, double maxY);

private:
    bool Init();
    bool Cleanup();
};

}  // namespace geo

#endif // TRANSFORMER_H
