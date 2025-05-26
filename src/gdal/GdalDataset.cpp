#include "GdalDataset.h"
#include "gdal_priv.h"
#include "cpl_conv.h"  // for CPLMalloc
#include <stdexcept>

GdalDataset::GdalDataset(const std::string& path, bool writeMode) : writeMode_(writeMode) {
    GDALAllRegister();
    dataset_ = writeMode
        ? static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_Update))
        : static_cast<GDALDataset*>(GDALOpen(path.c_str(), GA_ReadOnly));

    if (!dataset_) {
        throw std::runtime_error("Failed to open GDAL dataset: " + path);
    }
}

GdalDataset::~GdalDataset() {
    if (dataset_) {
        GDALClose(dataset_);
    }
}

std::string GdalDataset::getProjection() const {
    return dataset_->GetProjectionRef();
}

std::pair<double, double> GdalDataset::getGeoTransform() const {
    double transform[6];
    if (dataset_->GetGeoTransform(transform) == CE_None) {
        return { transform[1], transform[5] }; // pixel width, height
    } else {
        throw std::runtime_error("Failed to get geotransform.");
    }
}

std::pair<int, int> GdalDataset::getRasterSize() const {
    return { dataset_->GetRasterXSize(), dataset_->GetRasterYSize() };
}

std::string GdalDataset::getDriverName() const {
    return dataset_->GetDriver()->GetDescription();
}

GDALDataset* GdalDataset::raw() const {
    return dataset_;
}
