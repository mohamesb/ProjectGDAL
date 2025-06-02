#pragma once

#include "config/config.h"
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <memory>
#include <vector>
#include <string>

namespace geo {

class GdalDataset {
public:
    // RAII constructor/destructor
    GdalDataset();
    ~GdalDataset();
    
    // Disable copy, enable move
    GdalDataset(const GdalDataset&) = delete;
    GdalDataset& operator=(const GdalDataset&) = delete;
    GdalDataset(GdalDataset&&) = default;
    GdalDataset& operator=(GdalDataset&&) = default;
    
    // File operations
    bool open(const std::string& filename);
    bool create(const std::string& filename, const std::string& format, 
                int width, int height, int bands, GDALDataType dataType);
    void close();
    
    // Metadata access
    int getWidth() const;
    int getHeight() const;
    int getBandCount() const;
    std::string getProjection() const;
    BoundingBox getBounds() const;
    GDALDataType getDataType() const;
    double getNoDataValue(int band = 1) const;
    
    // Data access
    std::vector<double> readBand(int bandNumber) const;
    std::vector<double> readBand(int bandNumber, int xOff, int yOff, 
                                int xSize, int ySize) const;
    bool writeBand(int bandNumber, const std::vector<double>& data);
    bool writeBand(int bandNumber, const std::vector<double>& data,
                   int xOff, int yOff, int xSize, int ySize);
    
    // Transformation info
    std::vector<double> getGeoTransform() const;
    void setGeoTransform(const std::vector<double>& transform);
    void setProjection(const std::string& projection);
    void setNoDataValue(int band, double value);
    
    // Utility
    bool isValid() const { return dataset_ != nullptr; }
    GDALDataset* getRawDataset() const { return dataset_; }
    
private:
    GDALDataset* dataset_;
    static bool gdalInitialized_;
    
    static void initializeGdal();
    static void cleanupGdal();
};

// GDAL initialization manager
class GdalManager {
public:
    GdalManager();
    ~GdalManager();
    
    static GdalManager& getInstance();
    
private:
    static std::unique_ptr<GdalManager> instance_;
};

} // namespace geo
