#pragma once
#include <gdal_priv.h>
#include <string>
#include <memory>

class GdalDataset {
public:
    explicit GdalDataset(const std::string& path, bool writeMode = false);
    ~GdalDataset();

    std::string getProjection() const;
    std::pair<double, double> getGeoTransform() const;
    std::pair<int, int> getRasterSize() const;
    std::string getDriverName() const;

    // Optional: expose raw pointer if advanced use needed
    GDALDataset* raw() const;

private:
    GDALDataset* dataset_;
    bool writeMode_;
};
