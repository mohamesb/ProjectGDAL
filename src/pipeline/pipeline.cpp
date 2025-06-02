#include "pipeline/pipeline.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace geo {

Pipeline::Pipeline(const Config& config)
    : config_(config), verbose_(config.verbose), hasErrors_(false) {
    
    try {
        config_.validate();
    } catch (const std::exception& e) {
        logError("Configuration validation failed: " + std::string(e.what()));
        hasErrors_ = true;
    }
}

bool Pipeline::run() {
    logStep("Starting geospatial processing pipeline");
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Execute pipeline steps in sequence
        if (!load()) {
            logError("Load step failed");
            return false;
        }
        
        if (!clean()) {
            logError("Clean step failed");
            return false;
        }
        
        if (!transform()) {
            logError("Transform step failed");
            return false;
        }
        
        if (!save()) {
            logError("Save step failed");
            return false;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        logInfo("Pipeline completed successfully in " + std::to_string(duration.count()) + "ms");
        return true;
        
    } catch (const std::exception& e) {
        logError("Pipeline execution failed: " + std::string(e.what()));
        return false;
    }
}

bool Pipeline::load() {
    logStep("Loading input dataset");
    
    if (!validateInputFile()) {
        return false;
    }
    
    inputDataset_ = std::make_unique<GdalDataset>();
    
    if (!inputDataset_->open(config_.inputFile)) {
        logError("Failed to open input file: " + config_.inputFile);
        return false;
    }
    
    if (verbose_) {
        printDatasetInfo(*inputDataset_, "Input dataset");
    }
    
    logInfo("Successfully loaded input dataset");
    return true;
}

bool Pipeline::clean() {
    logStep("Cleaning dataset");
    
    if (!inputDataset_ || !inputDataset_->isValid()) {
        logError("No valid input dataset for cleaning");
        return false;
    }
    
    // For now, cleaning just validates the dataset
    // Additional cleaning operations can be added here
    
    if (inputDataset_->getBandCount() == 0) {
        logError("Dataset has no raster bands");
        return false;
    }
    
    if (inputDataset_->getWidth() <= 0 || inputDataset_->getHeight() <= 0) {
        logError("Dataset has invalid dimensions");
        return false;
    }
    
    // Check for and report nodata values
    if (verbose_) {
        for (int band = 1; band <= inputDataset_->getBandCount(); ++band) {
            double nodataValue = inputDataset_->getNoDataValue(band);
            if (nodataValue != 0.0) {
                logInfo("Band " + std::to_string(band) + " nodata value: " + std::to_string(nodataValue));
            }
        }
    }
    
    logInfo("Dataset cleaning completed");
    return true;
}

bool Pipeline::transform() {
    logStep("Transforming dataset");
    
    if (!inputDataset_ || !inputDataset_->isValid()) {
        logError("No valid input dataset for transformation");
        return false;
    }
    
    // Check if any transformations are needed
    bool needsTransformation = config_.targetCRS.has_value() || 
                              config_.clipBounds.has_value() || 
                              config_.applyNodataMask;
    
    if (!needsTransformation) {
        logInfo("No transformations specified, using original dataset");
        workingDataset_ = std::make_unique<GdalDataset>();
        
        // Create a copy of the input dataset
        std::string tempPath = "/tmp/geo_working.tif";
        if (!workingDataset_->create(tempPath, "GTiff", 
                                   inputDataset_->getWidth(), inputDataset_->getHeight(),
                                   inputDataset_->getBandCount(), inputDataset_->getDataType())) {
            logError("Failed to create working dataset copy");
            return false;
        }
        
        // Copy geotransform and projection
        auto geoTransform = inputDataset_->getGeoTransform();
        if (!geoTransform.empty()) {
            workingDataset_->setGeoTransform(geoTransform);
        }
        workingDataset_->setProjection(inputDataset_->getProjection());
        
        // Copy band data
        for (int band = 1; band <= inputDataset_->getBandCount(); ++band) {
            auto bandData = inputDataset_->readBand(band);
            if (!bandData.empty()) {
                workingDataset_->writeBand(band, bandData);
                double nodataValue = inputDataset_->getNoDataValue(band);
                if (nodataValue != 0.0) {
                    workingDataset_->setNoDataValue(band, nodataValue);
                }
            }
        }
        
        return true;
    }
    
    // Apply transformations
    workingDataset_ = transformer_.transformDataset(*inputDataset_, config_);
    
    if (!workingDataset_ || !workingDataset_->isValid()) {
        logError("Transformation failed");
        return false;
    }
    
    if (verbose_) {
        printDatasetInfo(*workingDataset_, "Transformed dataset");
    }
    
    logInfo("Dataset transformation completed");
    return true;
}

bool Pipeline::save() {
    logStep("Saving output dataset");
    
    if (!workingDataset_ || !workingDataset_->isValid()) {
        logError("No valid dataset to save");
        return false;
    }
    
    if (!validateOutputPath()) {
        return false;
    }
    
    // Create output dataset
    auto outputDataset = std::make_unique<GdalDataset>();
    
    if (!outputDataset->create(config_.outputFile, config_.outputFormat,
                              workingDataset_->getWidth(), workingDataset_->getHeight(),
                              workingDataset_->getBandCount(), workingDataset_->getDataType())) {
        logError("Failed to create output dataset: " + config_.outputFile);
        return false;
    }
    
    // Copy geotransform and projection
    auto geoTransform = workingDataset_->getGeoTransform();
    if (!geoTransform.empty()) {
        outputDataset->setGeoTransform(geoTransform);
    }
    
    std::string projection = workingDataset_->getProjection();
    if (!projection.empty()) {
        outputDataset->setProjection(projection);
    }
    
    // Copy band data
    for (int band = 1; band <= workingDataset_->getBandCount(); ++band) {
        auto bandData = workingDataset_->readBand(band);
        if (bandData.empty()) {
            logError("Failed to read band " + std::to_string(band) + " for output");
            return false;
        }
        
        if (!outputDataset->writeBand(band, bandData)) {
            logError("Failed to write band " + std::to_string(band) + " to output");
            return false;
        }
        
        // Copy nodata value
        double nodataValue = workingDataset_->getNoDataValue(band);
        if (nodataValue != 0.0) {
            outputDataset->setNoDataValue(band, nodataValue);
        }
    }
    
    // Force write to disk
    outputDataset->close();
    
    if (verbose_) {
        logInfo("Output saved to: " + config_.outputFile);
        logInfo("Output format: " + config_.outputFormat);
    }
    
    logInfo("Dataset saved successfully");
    return true;
}

void Pipeline::logInfo(const std::string& message) {
    if (verbose_) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] INFO: " 
                  << message << std::endl;
    }
}

void Pipeline::logError(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::cerr << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] ERROR: " 
              << message << std::endl;
    lastError_ = message;
    hasErrors_ = true;
}

void Pipeline::logStep(const std::string& step) {
    if (verbose_) {
        std::cout << "\n=== " << step << " ===" << std::endl;
    }
}

bool Pipeline::validateInputFile() {
    if (config_.inputFile.empty()) {
        logError("Input file path is empty");
        return false;
    }
    
    if (!std::filesystem::exists(config_.inputFile)) {
        logError("Input file does not exist: " + config_.inputFile);
        return false;
    }
    
    if (!std::filesystem::is_regular_file(config_.inputFile)) {
        logError("Input path is not a regular file: " + config_.inputFile);
        return false;
    }
    
    return true;
}

bool Pipeline::validateOutputPath() {
    if (config_.outputFile.empty()) {
        logError("Output file path is empty");
        return false;
    }
    
    // Check if output directory exists, create if needed
    std::filesystem::path outputPath(config_.outputFile);
    std::filesystem::path outputDir = outputPath.parent_path();
    
    if (!outputDir.empty() && !std::filesystem::exists(outputDir)) {
        try {
            std::filesystem::create_directories(outputDir);
            logInfo("Created output directory: " + outputDir.string());
        } catch (const std::exception& e) {
            logError("Failed to create output directory: " + std::string(e.what()));
            return false;
        }
    }
    
    // Check if output file already exists and warn
    if (std::filesystem::exists(config_.outputFile)) {
        logInfo("Output file already exists and will be overwritten: " + config_.outputFile);
    }
    
    return true;
}

void Pipeline::printDatasetInfo(const GdalDataset& dataset, const std::string& label) {
    std::cout << "\n--- " << label << " ---" << std::endl;
    std::cout << "Dimensions: " << dataset.getWidth() << " x " << dataset.getHeight() << std::endl;
    std::cout << "Bands: " << dataset.getBandCount() << std::endl;
    std::cout << "Data type: " << GDALGetDataTypeName(dataset.getDataType()) << std::endl;
    
    auto bounds = dataset.getBounds();
    std::cout << "Bounds: [" << bounds.minX << ", " << bounds.minY 
              << ", " << bounds.maxX << ", " << bounds.maxY << "]" << std::endl;
    
    std::string projection = dataset.getProjection();
    if (!projection.empty()) {
        // Print just the first 100 characters of projection to avoid clutter
        if (projection.length() > 100) {
            projection = projection.substr(0, 97) + "...";
        }
        std::cout << "Projection: " << projection << std::endl;
    }
}

void Pipeline::cleanup() {
    inputDataset_.reset();
    workingDataset_.reset();
}

} // namespace geo