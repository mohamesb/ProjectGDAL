#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

using json = nlohmann::json;

namespace geo {

bool Config::isValid() const {
    return !inputFile.empty() && !outputFile.empty();
}

void Config::validate() const {
    if (inputFile.empty()) {
        throw std::invalid_argument("Input file path is required");
    }
    
    if (outputFile.empty()) {
        throw std::invalid_argument("Output file path is required");
    }
    
    if (!std::filesystem::exists(inputFile)) {
        throw std::invalid_argument("Input file does not exist: " + inputFile);
    }
    
    // Validate bounding box if provided
    if (clipBounds.has_value()) {
        const auto& bbox = clipBounds.value();
        if (bbox.minX >= bbox.maxX || bbox.minY >= bbox.maxY) {
            throw std::invalid_argument("Invalid bounding box: min values must be less than max values");
        }
    }
}


// Implementation of the helper function
Config ConfigLoader::parseConfigFromJson(const json& configJson) {
    Config config;
    
    // Required fields
    if (configJson.contains("input_file")) {
        config.inputFile = configJson["input_file"];
    }
    if (configJson.contains("output_file")) {
        config.outputFile = configJson["output_file"];
    }
    
    // Optional fields
    if (configJson.contains("output_format")) {
        config.outputFormat = configJson["output_format"];
    }
    if (configJson.contains("target_crs")) {
        config.targetCRS = configJson["target_crs"];
    }
    if (configJson.contains("verbose")) {
        config.verbose = configJson["verbose"];
    }
    if (configJson.contains("apply_nodata_mask")) {
        config.applyNodataMask = configJson["apply_nodata_mask"];
    }
    if (configJson.contains("nodata_value")) {
        config.nodataValue = configJson["nodata_value"];
    }
    if (configJson.contains("compression_level")) {
        config.compressionLevel = configJson["compression_level"];
    }
    
    // Bounding box
    if (configJson.contains("clip_bounds")) {
        const auto& bbox = configJson["clip_bounds"];
        if (bbox.contains("min_x") && bbox.contains("min_y") && 
            bbox.contains("max_x") && bbox.contains("max_y")) {
            config.clipBounds = BoundingBox(
                bbox["min_x"], bbox["min_y"],
                bbox["max_x"], bbox["max_y"]
            );
        }
    }
    
    return config;
}

// Add this debug version to your config.cpp to identify where the error occurs

Config ConfigLoader::loadFromJson(const std::string& configPath, int index) {
    std::cout << "DEBUG: loadFromJson called with path='" << configPath << "', index=" << index << std::endl;
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + configPath);
    }
    
    json j;
    try {
        file >> j;
        std::cout << "DEBUG: JSON loaded successfully" << std::endl;
        std::cout << "DEBUG: JSON is_array() = " << j.is_array() << std::endl;
        std::cout << "DEBUG: JSON is_object() = " << j.is_object() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "DEBUG: JSON parsing failed: " << e.what() << std::endl;
        throw;
    }
    
    // Handle both single object and array of objects
    json configJson;
    if (j.is_array()) {
        std::cout << "DEBUG: Processing as array, size=" << j.size() << std::endl;
        if (index >= j.size()) {
            throw std::invalid_argument("Config index " + std::to_string(index) + 
                                      " is out of range. Array has " + std::to_string(j.size()) + " elements.");
        }
        configJson = j[index];
    } else {
        std::cout << "DEBUG: Processing as single object" << std::endl;
        if (index != 0) {
            throw std::invalid_argument("Config index " + std::to_string(index) + 
                                      " specified but config file contains single object.");
        }
        configJson = j;
    }
    
    std::cout << "DEBUG: About to parse config fields" << std::endl;
    
    Config config;
    
    // Required fields
    if (configJson.contains("input_file")) {
        config.inputFile = configJson["input_file"];
        std::cout << "DEBUG: Set input_file = " << config.inputFile << std::endl;
    }
    if (configJson.contains("output_file")) {
        config.outputFile = configJson["output_file"];
        std::cout << "DEBUG: Set output_file = " << config.outputFile << std::endl;
    }
    
    // Optional fields
    if (configJson.contains("output_format")) {
        config.outputFormat = configJson["output_format"];
    }
    if (configJson.contains("target_crs")) {
        config.targetCRS = configJson["target_crs"];
    }
    if (configJson.contains("verbose")) {
        config.verbose = configJson["verbose"];
    }
    if (configJson.contains("apply_nodata_mask")) {
        config.applyNodataMask = configJson["apply_nodata_mask"];
    }
    if (configJson.contains("nodata_value")) {
        config.nodataValue = configJson["nodata_value"];
    }
    if (configJson.contains("compression_level")) {
        config.compressionLevel = configJson["compression_level"];
    }
    
    // Bounding box
    if (configJson.contains("clip_bounds")) {
        std::cout << "DEBUG: Processing clip_bounds" << std::endl;
        const auto& bbox = configJson["clip_bounds"];
        if (bbox.contains("min_x") && bbox.contains("min_y") && 
            bbox.contains("max_x") && bbox.contains("max_y")) {
            config.clipBounds = BoundingBox(
                bbox["min_x"], bbox["min_y"],
                bbox["max_x"], bbox["max_y"]
            );
            std::cout << "DEBUG: clip_bounds set successfully" << std::endl;
        }
    }
    
    std::cout << "DEBUG: loadFromJson completed successfully" << std::endl;
    return config;
}

// Also add debug to loadAllFromJson in case it's being called
std::vector<Config> ConfigLoader::loadAllFromJson(const std::string& configPath) {
    std::cout << "DEBUG: loadAllFromJson called with path='" << configPath << "'" << std::endl;
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + configPath);
    }
    
    json j;
    try {
        file >> j;
        std::cout << "DEBUG: JSON loaded in loadAllFromJson" << std::endl;
        std::cout << "DEBUG: JSON is_array() = " << j.is_array() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "DEBUG: JSON parsing failed in loadAllFromJson: " << e.what() << std::endl;
        throw;
    }
    
    std::vector<Config> configs;
    
    if (j.is_array()) {
        std::cout << "DEBUG: Processing array with " << j.size() << " elements" << std::endl;
        for (size_t i = 0; i < j.size(); ++i) {
            // Parse each config directly instead of calling loadFromJson recursively
            Config config;
            const auto& configJson = j[i];
            
            // Parse fields directly (same as in loadFromJson)
            if (configJson.contains("input_file")) {
                config.inputFile = configJson["input_file"];
            }
            if (configJson.contains("output_file")) {
                config.outputFile = configJson["output_file"];
            }
            // ... (rest of the parsing logic)
            
            configs.push_back(config);
        }
    } else {
        std::cout << "DEBUG: Processing single object in loadAllFromJson" << std::endl;
        // Call loadFromJson for single object
        configs.push_back(loadFromJson(configPath, 0));
    }
    
    std::cout << "DEBUG: loadAllFromJson completed, returning " << configs.size() << " configs" << std::endl;
    return configs;
}

// Fixed loadFromArgs function (add this to your config.cpp)
Config ConfigLoader::loadFromArgs(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                config.inputFile = argv[++i];
            } else {
                throw std::invalid_argument("Missing value for " + arg);
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                config.outputFile = argv[++i];
            } else {
                throw std::invalid_argument("Missing value for " + arg);
            }
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc) {
                config.outputFormat = argv[++i];
            } else {
                throw std::invalid_argument("Missing value for " + arg);
            }
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                std::string configPath = argv[++i];
                // Make sure we got a valid path, not another flag
                if (configPath.empty() || configPath[0] == '-') {
                    throw std::invalid_argument("Invalid config file path: " + configPath);
                }
                return loadFromJson(configPath, 0);
            } else {
                throw std::invalid_argument("Missing config file path after " + arg);
            }
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--target-crs") {
            if (i + 1 < argc) {
                config.targetCRS = argv[++i];
            } else {
                throw std::invalid_argument("Missing value for " + arg);
            }
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }
    
    return config;
}

void ConfigLoader::printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "Options:\n"
              << "  -i, --input FILE      Input geospatial file\n"
              << "  -o, --output FILE     Output file path\n"
              << "  -f, --format FORMAT   Output format (default: GTiff)\n"
              << "  -c, --config FILE     JSON configuration file\n"
              << "  --target-crs CRS      Target coordinate reference system\n"
              << "  -v, --verbose         Enable verbose output\n"
              << "  -h, --help            Show this help message\n";
}

} // namespace geo