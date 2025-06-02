#include "gdal/GdalDataset.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace geo {

// Static member initialization
bool GdalDataset::gdalInitialized_ = false;
std::unique_ptr<GdalManager> GdalManager::instance_ = nullptr;

// GdalManager implementation
GdalManager::GdalManager() {
    GDALAllRegister();
    OGRRegisterAll();
    std::cout << "GDAL initialized. Version: " << GDALVersionInfo("RELEASE_NAME") << std::endl;
}

GdalManager::~GdalManager() {
    GDALDestroyDriverManager();
    std::cout << "GDAL cleanup completed." << std::endl;
}

GdalManager& GdalManager::getInstance() {
    if (!instance_) {
        instance_ = std::make_unique<GdalManager>();
    }
    return *instance_;
}

// GdalDataset implementation
GdalDataset::GdalDataset() : dataset_(nullptr) {
    initializeGdal();
}

GdalDataset::~GdalDataset() {
    close();
}

void GdalDataset::initializeGdal() {
    if (!gdalInitialized_) {
        GdalManager::getInstance(); // Ensures GDAL is initialized
        gdalInitialized_ = true;
    }
}

void GdalDataset::cleanupGdal() {
    // Cleanup is handled by GdalManager destructor
}

bool GdalDataset::open(const std::string& filename) {
    close(); // Close any existing dataset
    
    dataset_ = static_cast<GDALDataset*>(GDALOpen(filename.c_str(), GA_ReadOnly));
    
    if (!dataset_) {
        std::cerr << "Failed to open dataset: " << filename << std::endl;
        std::cerr << "GDAL Error: " << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    
    return true;
}

bool GdalDataset::create(const std::string& filename, const std::string& format,
                        int width, int height, int bands, GDALDataType dataType) {
    close(); // Close any existing dataset
    
    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(format.c_str());
    if (!driver) {
        std::cerr << "Driver not found: " << format << std::endl;
        return false;
    }
    
    dataset_ = driver->Create(filename.c_str(), width, height, bands, dataType, nullptr);
    
    if (!dataset_) {
        std::cerr << "Failed to create dataset: " << filename << std::endl;
        std::cerr << "GDAL Error: " << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    
    return true;
}

void GdalDataset::close() {
    if (dataset_) {
        GDALClose(dataset_);
        dataset_ = nullptr;
    }
}

int GdalDataset::getWidth() const {
    return dataset_ ? dataset_->GetRasterXSize() : 0;
}

int GdalDataset::getHeight() const {
    return dataset_ ? dataset_->GetRasterYSize() : 0;
}

int GdalDataset::getBandCount() const {
    return dataset_ ? dataset_->GetRasterCount() : 0;
}

std::string GdalDataset::getProjection() const {
    if (!dataset_) return "";
    
    const char* proj = dataset_->GetProjectionRef();
    return proj ? std::string(proj) : "";
}

BoundingBox GdalDataset::getBounds() const {
    if (!dataset_) return BoundingBox();
    
    double transform[6];
    if (dataset_->GetGeoTransform(transform) != CE_None) {
        return BoundingBox();
    }
    
    double minX = transform[0];
    double maxX = transform[0] + getWidth() * transform[1];
    double maxY = transform[3];
    double minY = transform[3] + getHeight() * transform[5];
    
    return BoundingBox(minX, minY, maxX, maxY);
}

GDALDataType GdalDataset::getDataType() const {
    if (!dataset_ || getBandCount() == 0) return GDT_Unknown;
    
    GDALRasterBand* band = dataset_->GetRasterBand(1);
    return band ? band->GetRasterDataType() : GDT_Unknown;
}

double GdalDataset::getNoDataValue(int band) const {
    if (!dataset_ || band < 1 || band > getBandCount()) return 0.0;
    
    GDALRasterBand* rasterBand = dataset_->GetRasterBand(band);
    if (!rasterBand) return 0.0;
    
    int hasNoData;
    double noDataValue = rasterBand->GetNoDataValue(&hasNoData);
    return hasNoData ? noDataValue : 0.0;
}

std::vector<double> GdalDataset::readBand(int bandNumber) const {
    return readBand(bandNumber, 0, 0, getWidth(), getHeight());
}

std::vector<double> GdalDataset::readBand(int bandNumber, int xOff, int yOff,
                                         int xSize, int ySize) const {
    if (!dataset_ || bandNumber < 1 || bandNumber > getBandCount()) {
        return {};
    }
    
    GDALRasterBand* band = dataset_->GetRasterBand(bandNumber);
    if (!band) return {};
    
    std::vector<double> data(xSize * ySize);
    
    CPLErr err = band->RasterIO(GF_Read, xOff, yOff, xSize, ySize,
                               data.data(), xSize, ySize, GDT_Float64, 0, 0);
    
    if (err != CE_None) {
        std::cerr << "Error reading raster data: " << CPLGetLastErrorMsg() << std::endl;
        return {};
    }
    
    return data;
}

bool GdalDataset::writeBand(int bandNumber, const std::vector<double>& data) {
    return writeBand(bandNumber, data, 0, 0, getWidth(), getHeight());
}

bool GdalDataset::writeBand(int bandNumber, const std::vector<double>& data,
                           int xOff, int yOff, int xSize, int ySize) {
    if (!dataset_ || bandNumber < 1 || bandNumber > getBandCount()) {
        return false;
    }
    
    if (data.size() != static_cast<size_t>(xSize * ySize)) {
        std::cerr << "Data size mismatch for band write operation" << std::endl;
        return false;
    }
    
    GDALRasterBand* band = dataset_->GetRasterBand(bandNumber);
    if (!band) return false;
    
    CPLErr err = band->RasterIO(GF_Write, xOff, yOff, xSize, ySize,
                               const_cast<double*>(data.data()), xSize, ySize,
                               GDT_Float64, 0, 0);
    
    if (err != CE_None) {
        std::cerr << "Error writing raster data: " << CPLGetLastErrorMsg() << std::endl;
        return false;
    }
    
    return true;
}

std::vector<double> GdalDataset::getGeoTransform() const {
    if (!dataset_) return {};
    
    double transform[6];
    if (dataset_->GetGeoTransform(transform) != CE_None) {
        return {};
    }
    
    return std::vector<double>(transform, transform + 6);
}

void GdalDataset::setGeoTransform(const std::vector<double>& transform) {
    if (!dataset_ || transform.size() != 6) return;
    
    dataset_->SetGeoTransform(const_cast<double*>(transform.data()));
}

void GdalDataset::setProjection(const std::string& projection) {
    if (!dataset_) return;
    
    dataset_->SetProjection(projection.c_str());
}

void GdalDataset::setNoDataValue(int band, double value) {
    if (!dataset_ || band < 1 || band > getBandCount()) return;
    
    GDALRasterBand* rasterBand = dataset_->GetRasterBand(band);
    if (rasterBand) {
        rasterBand->SetNoDataValue(value);
    }
}

} // namespace geo
