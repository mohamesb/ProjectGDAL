#include "transform/transformer.h"
#include <gdalwarper.h>
#include <ogr_spatialref.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <cmath>

namespace geo {

std::unique_ptr<GdalDataset> Transformer::reprojectDataset(const GdalDataset& input,
                                                          const std::string& targetCRS) {
    if (!input.isValid()) {
        std::cerr << "Invalid input dataset for reprojection" << std::endl;
        return nullptr;
    }
    
    std::string tempPath = generateTempFilename();
    
    // Create output spatial reference
    OGRSpatialReference targetSRS;
    if (targetSRS.SetFromUserInput(targetCRS.c_str()) != OGRERR_NONE) {
        std::cerr << "Failed to parse target CRS: " << targetCRS << std::endl;
        return nullptr;
    }
    char* targetWkt = nullptr;
    targetSRS.exportToWkt(&targetWkt);

    // Set up warp options
    GDALWarpOptions* warpOptions = GDALCreateWarpOptions();
    warpOptions->hSrcDS = input.getRawDataset();
    warpOptions->nBandCount = input.getBandCount();
    warpOptions->panSrcBands = static_cast<int*>(CPLMalloc(sizeof(int) * warpOptions->nBandCount));
    warpOptions->panDstBands = static_cast<int*>(CPLMalloc(sizeof(int) * warpOptions->nBandCount));
    for (int i = 0; i < warpOptions->nBandCount; ++i) {
        warpOptions->panSrcBands[i] = i + 1;
        warpOptions->panDstBands[i] = i + 1;
    }

    // Create coordinate transformation
    warpOptions->pTransformerArg = GDALCreateGenImgProjTransformer(
        input.getRawDataset(), input.getProjection().c_str(),
        nullptr, targetWkt,
        FALSE, 0.0, 1);
    if (!warpOptions->pTransformerArg) {
        std::cerr << "Failed to create coordinate transformer" << std::endl;
        CPLFree(targetWkt);
        GDALDestroyWarpOptions(warpOptions);
        return nullptr;
    }
    warpOptions->pfnTransformer = GDALGenImgProjTransform;

    // Suggest output file size and geotransform
    double geoTransform[6];
    int pixelCount, lineCount;
    if (GDALSuggestedWarpOutput(input.getRawDataset(),
                            GDALGenImgProjTransform, warpOptions->pTransformerArg,
                            geoTransform, &pixelCount, &lineCount) != CE_None) {
        std::cerr << "Failed to determine output dimensions" << std::endl;
        CPLFree(targetWkt);
        GDALDestroyTransformer(warpOptions->pTransformerArg);
        GDALDestroyWarpOptions(warpOptions);
        return nullptr;
    }

    // Create output dataset
    auto outputDataset = std::make_unique<GdalDataset>();
    if (!outputDataset->create(tempPath, "GTiff", pixelCount, lineCount,
                            input.getBandCount(), input.getDataType())) {
        std::cerr << "Failed to create output dataset for reprojection" << std::endl;
        CPLFree(targetWkt);
        GDALDestroyTransformer(warpOptions->pTransformerArg);
        GDALDestroyWarpOptions(warpOptions);
        return nullptr;
    }

    // Set output geotransform and projection
    std::vector<double> outGeoTransform(geoTransform, geoTransform + 6);
    outputDataset->setGeoTransform(outGeoTransform);
    outputDataset->setProjection(targetWkt);

    // Set up warp options for output
    warpOptions->hDstDS = outputDataset->getRawDataset();

    // Perform the warp
    GDALWarpOperation warpOp;
    if (warpOp.Initialize(warpOptions) != CE_None) {
        std::cerr << "Failed to initialize warp operation" << std::endl;
        CPLFree(targetWkt);
        GDALDestroyTransformer(warpOptions->pTransformerArg);
        GDALDestroyWarpOptions(warpOptions);
        return nullptr;
    }
    if (warpOp.ChunkAndWarpImage(0, 0, pixelCount, lineCount) != CE_None) {
        std::cerr << "Warp operation failed" << std::endl;
        CPLFree(targetWkt);
        GDALDestroyTransformer(warpOptions->pTransformerArg);
        GDALDestroyWarpOptions(warpOptions);
        return nullptr;
    }

    // Cleanup
    CPLFree(targetWkt);
    GDALDestroyTransformer(warpOptions->pTransformerArg);
    GDALDestroyWarpOptions(warpOptions);
    
    return outputDataset;
}

std::unique_ptr<GdalDataset> Transformer::clipDataset(const GdalDataset& input,
                                                     const BoundingBox& bounds) {
    if (!input.isValid()) {
        std::cerr << "Invalid input dataset for clipping" << std::endl;
        return nullptr;
    }
    
    int xOff, yOff, xSize, ySize;
    if (!calculateClipWindow(input, bounds, xOff, yOff, xSize, ySize)) {
        std::cerr << "Failed to calculate clip window" << std::endl;
        return nullptr;
    }
    
    std::string tempPath = generateTempFilename();
    auto outputDataset = std::make_unique<GdalDataset>();
    
    if (!outputDataset->create(tempPath, "GTiff", xSize, ySize,
                              input.getBandCount(), input.getDataType())) {
        std::cerr << "Failed to create clipped output dataset" << std::endl;
        return nullptr;
    }
    
    // Copy geotransform and adjust for clipping offset
    auto geoTransform = input.getGeoTransform();
    if (!geoTransform.empty()) {
        geoTransform[0] += xOff * geoTransform[1]; // Adjust X origin
        geoTransform[3] += yOff * geoTransform[5]; // Adjust Y origin
        outputDataset->setGeoTransform(geoTransform);
    }
    
    // Copy projection
    outputDataset->setProjection(input.getProjection());
    
    // Copy band data
    for (int band = 1; band <= input.getBandCount(); ++band) {
        auto bandData = input.readBand(band, xOff, yOff, xSize, ySize);
        if (bandData.empty()) {
            std::cerr << "Failed to read band " << band << " for clipping" << std::endl;
            return nullptr;
        }
        
        if (!outputDataset->writeBand(band, bandData)) {
            std::cerr << "Failed to write clipped band " << band << std::endl;
            return nullptr;
        }
        
        // Copy nodata value
        double nodataValue = input.getNoDataValue(band);
        if (nodataValue != 0.0) {
            outputDataset->setNoDataValue(band, nodataValue);
        }
    }
    
    return outputDataset;
}

std::unique_ptr<GdalDataset> Transformer::applyNodataMask(const GdalDataset& input,
                                                         double nodataValue) {
    if (!input.isValid()) {
        std::cerr << "Invalid input dataset for nodata masking" << std::endl;
        return nullptr;
    }
    
    std::string tempPath = generateTempFilename();
    auto outputDataset = std::make_unique<GdalDataset>();
    
    if (!outputDataset->create(tempPath, "GTiff", input.getWidth(), input.getHeight(),
                              input.getBandCount(), input.getDataType())) {
        std::cerr << "Failed to create output dataset for nodata masking" << std::endl;
        return nullptr;
    }
    
    // Copy metadata
    copyMetadata(input, *outputDataset);
    
    // Process each band
    for (int band = 1; band <= input.getBandCount(); ++band) {
        auto bandData = input.readBand(band);
        if (bandData.empty()) {
            std::cerr << "Failed to read band " << band << " for nodata masking" << std::endl;
            return nullptr;
        }
        
        // Apply nodata mask - replace extreme values or invalid data
        for (auto& pixel : bandData) {
            if (std::isnan(pixel) || std::isinf(pixel) || 
                pixel < -1e10 || pixel > 1e10) {
                pixel = nodataValue;
            }
        }
        
        if (!outputDataset->writeBand(band, bandData)) {
            std::cerr << "Failed to write masked band " << band << std::endl;
            return nullptr;
        }
        
        outputDataset->setNoDataValue(band, nodataValue);
    }
    
    return outputDataset;
}

std::unique_ptr<GdalDataset> Transformer::scaleDataset(const GdalDataset& input,
                                                      double scaleFactor) {
    if (!input.isValid()) {
        std::cerr << "Invalid input dataset for scaling" << std::endl;
        return nullptr;
    }
    
    if (scaleFactor <= 0.0 || scaleFactor == 1.0) {
        std::cerr << "Invalid scale factor: " << scaleFactor << std::endl;
        return nullptr;
    }
    
    std::string tempPath = generateTempFilename();
    auto outputDataset = std::make_unique<GdalDataset>();
    
    if (!outputDataset->create(tempPath, "GTiff", input.getWidth(), input.getHeight(),
                              input.getBandCount(), input.getDataType())) {
        std::cerr << "Failed to create output dataset for scaling" << std::endl;
        return nullptr;
    }
    
    // Copy metadata
    copyMetadata(input, *outputDataset);
    
    // Process each band
    for (int band = 1; band <= input.getBandCount(); ++band) {
        auto bandData = input.readBand(band);
        if (bandData.empty()) {
            std::cerr << "Failed to read band " << band << " for scaling" << std::endl;
            return nullptr;
        }
        
        double nodataValue = input.getNoDataValue(band);
        
        // Apply scaling, preserving nodata values
        for (auto& pixel : bandData) {
            if (pixel != nodataValue && !std::isnan(pixel)) {
                pixel *= scaleFactor;
            }
        }
        
        if (!outputDataset->writeBand(band, bandData)) {
            std::cerr << "Failed to write scaled band " << band << std::endl;
            return nullptr;
        }
        
        if (nodataValue != 0.0) {
            outputDataset->setNoDataValue(band, nodataValue);
        }
    }
    
    return outputDataset;
}

std::unique_ptr<GdalDataset> Transformer::transformDataset(const GdalDataset& input,
                                                          const Config& config) {
    if (!input.isValid()) {
        std::cerr << "Invalid input dataset for transformation" << std::endl;
        return nullptr;
    }
    
    std::unique_ptr<GdalDataset> current = std::make_unique<GdalDataset>();
    
    // Create a copy of the input as starting point
    std::string tempPath = generateTempFilename();
    if (!current->create(tempPath, "GTiff", input.getWidth(), input.getHeight(),
                        input.getBandCount(), input.getDataType())) {
        std::cerr << "Failed to create initial working dataset" << std::endl;
        return nullptr;
    }
    
    copyMetadata(input, *current);
    
    // Copy all band data
    for (int band = 1; band <= input.getBandCount(); ++band) {
        auto bandData = input.readBand(band);
        if (!bandData.empty()) {
            current->writeBand(band, bandData);
            double nodataValue = input.getNoDataValue(band);
            if (nodataValue != 0.0) {
                current->setNoDataValue(band, nodataValue);
            }
        }
    }
    
    // Apply transformations in sequence
    
    // 1. Reprojection (if target CRS specified)
    if (config.targetCRS.has_value()) {
        if (config.verbose) {
            std::cout << "Reprojecting to: " << config.targetCRS.value() << std::endl;
        }
        current = reprojectDataset(*current, config.targetCRS.value());
        if (!current) {
            std::cerr << "Reprojection failed" << std::endl;
            return nullptr;
        }
    }
    
    // 2. Clipping (if bounds specified)
    if (config.clipBounds.has_value()) {
        if (config.verbose) {
            const auto& bounds = config.clipBounds.value();
            std::cout << "Clipping to bounds: [" << bounds.minX << ", " << bounds.minY 
                     << ", " << bounds.maxX << ", " << bounds.maxY << "]" << std::endl;
        }
        current = clipDataset(*current, config.clipBounds.value());
        if (!current) {
            std::cerr << "Clipping failed" << std::endl;
            return nullptr;
        }
    }
    
    // 3. Apply nodata mask (if enabled)
    if (config.applyNodataMask) {
        if (config.verbose) {
            std::cout << "Applying nodata mask with value: " << config.nodataValue << std::endl;
        }
        current = applyNodataMask(*current, config.nodataValue);
        if (!current) {
            std::cerr << "Nodata masking failed" << std::endl;
            return nullptr;
        }
    }
    
    return current;
}

bool Transformer::calculateClipWindow(const GdalDataset& dataset,
                                     const BoundingBox& bounds,
                                     int& xOff, int& yOff, int& xSize, int& ySize) {
    auto geoTransform = dataset.getGeoTransform();
    if (geoTransform.empty()) {
        std::cerr << "Dataset has no geotransform information" << std::endl;
        return false;
    }
    
    // Convert geographic bounds to pixel coordinates
    double pixelWidth = geoTransform[1];
    double pixelHeight = std::abs(geoTransform[5]);
    double originX = geoTransform[0];
    double originY = geoTransform[3];
    
    xOff = static_cast<int>((bounds.minX - originX) / pixelWidth);
    yOff = static_cast<int>((originY - bounds.maxY) / pixelHeight);
    
    int xMax = static_cast<int>((bounds.maxX - originX) / pixelWidth);
    int yMax = static_cast<int>((originY - bounds.minY) / pixelHeight);
    
    xSize = xMax - xOff;
    ySize = yMax - yOff;
    
    // Clamp to dataset bounds
    xOff = std::max(0, std::min(xOff, dataset.getWidth() - 1));
    yOff = std::max(0, std::min(yOff, dataset.getHeight() - 1));
    xSize = std::max(1, std::min(xSize, dataset.getWidth() - xOff));
    ySize = std::max(1, std::min(ySize, dataset.getHeight() - yOff));
    
    return xSize > 0 && ySize > 0;
}

std::unique_ptr<GdalDataset> Transformer::createOutputDataset(const std::string& tempPath,
                                                             const std::string& format,
                                                             int width, int height,
                                                             int bands, GDALDataType dataType) {
    auto dataset = std::make_unique<GdalDataset>();
    if (!dataset->create(tempPath, format, width, height, bands, dataType)) {
        return nullptr;
    }
    return dataset;
}

void Transformer::copyMetadata(const GdalDataset& source, GdalDataset& target) {
    // Copy geotransform
    auto geoTransform = source.getGeoTransform();
    if (!geoTransform.empty()) {
        target.setGeoTransform(geoTransform);
    }
    
    // Copy projection
    std::string projection = source.getProjection();
    if (!projection.empty()) {
        target.setProjection(projection);
    }
}

bool Transformer::transformBounds(const BoundingBox& input, const std::string& sourceCRS,
                                 const std::string& targetCRS, BoundingBox& output) {
    OGRSpatialReference sourceSRS, targetSRS;
    
    if (sourceSRS.SetFromUserInput(sourceCRS.c_str()) != OGRERR_NONE ||
        targetSRS.SetFromUserInput(targetCRS.c_str()) != OGRERR_NONE) {
        return false;
    }
    
    OGRCoordinateTransformation* transform = 
        OGRCreateCoordinateTransformation(&sourceSRS, &targetSRS);
    
    if (!transform) {
        return false;
    }
    
    double x1 = input.minX, y1 = input.minY;
    double x2 = input.maxX, y2 = input.maxY;
    
    if (transform->Transform(1, &x1, &y1) && transform->Transform(1, &x2, &y2)) {
        output.minX = std::min(x1, x2);
        output.maxX = std::max(x1, x2);
        output.minY = std::min(y1, y2);
        output.maxY = std::max(y1, y2);
        
        OCTDestroyCoordinateTransformation(transform);
        return true;
    }
    
    OCTDestroyCoordinateTransformation(transform);
    return false;
}

std::string Transformer::generateTempFilename() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    
    std::ostringstream oss;
    oss << "/tmp/geo_temp_" << dis(gen) << ".tif";
    return oss.str();
}

} // namespace geo

