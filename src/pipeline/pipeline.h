#pragma once

#include "config/config.h"
#include "gdal/GdalDataset.h"
#include "transform/transformer.h"
#include <memory>
#include <string>

namespace geo {

class Pipeline {
public:
    Pipeline(const Config& config);
    ~Pipeline() = default;
    
    // Disable copy, enable move
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = default;
    Pipeline& operator=(Pipeline&&) = default;
    
    // Main pipeline execution
    bool run();
    
    // Individual pipeline steps
    bool load();
    bool clean();
    bool transform();
    bool save();
    
    // Status and logging
    void setVerbose(bool verbose) { verbose_ = verbose; }
    bool hasErrors() const { return hasErrors_; }
    const std::string& getLastError() const { return lastError_; }
    
private:
    Config config_;
    std::unique_ptr<GdalDataset> inputDataset_;
    std::unique_ptr<GdalDataset> workingDataset_;
    Transformer transformer_;
    
    bool verbose_;
    bool hasErrors_;
    std::string lastError_;
    
    // Helper methods
    void logInfo(const std::string& message);
    void logError(const std::string& message);
    void logStep(const std::string& step);
    
    bool validateInputFile();
    bool validateOutputPath();
    void printDatasetInfo(const GdalDataset& dataset, const std::string& label);
    
    // Cleanup
    void cleanup();
};

} // namespace geo
