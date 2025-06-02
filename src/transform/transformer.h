#pragma once

#include "config/config.h"
#include "gdal/GdalDataset.h"
#include <memory>

namespace geo {

class Transformer {
public:
    Transformer() = default;
    ~Transformer() = default;
    
    // Disable copy, enable move
    Transformer(const Transformer&) = delete;
    Transformer& operator=(const Transformer&) = delete;
    Transformer(Transformer&&) = default;
    Transformer& operator=(Transformer&&) = default;
    
    // Main transformation methods
    std::unique_ptr<GdalDataset> reprojectDataset(const GdalDataset& input,
                                                 const std::string& targetCRS);
    
    std::unique_ptr<GdalDataset> clipDataset(const GdalDataset& input,
                                            const BoundingBox& bounds);
    
    std::unique_ptr<GdalDataset> applyNodataMask(const GdalDataset& input,
                                                double nodataValue);
    
    std::unique_ptr<GdalDataset> scaleDataset(const GdalDataset& input,
                                             double scaleFactor);
    
    // Combined transformation pipeline
    std::unique_ptr<GdalDataset> transformDataset(const GdalDataset& input,
                                                 const Config& config);
    
private:
    // Helper methods
    bool calculateClipWindow(const GdalDataset& dataset,
                           const BoundingBox& bounds,
                           int& xOff, int& yOff, int& xSize, int& ySize);
    
    std::unique_ptr<GdalDataset> createOutputDataset(const std::string& tempPath,
                                                    const std::string& format,
                                                    int width, int height,
                                                    int bands, GDALDataType dataType);
    
    void copyMetadata(const GdalDataset& source, GdalDataset& target);
    
    // Coordinate transformation utilities
    bool transformBounds(const BoundingBox& input, const std::string& sourceCRS,
                        const std::string& targetCRS, BoundingBox& output);
    
    std::string generateTempFilename() const;
};

} // namespace geo