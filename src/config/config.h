#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geo {

struct BoundingBox {
    double minX, minY, maxX, maxY;
    
    BoundingBox() : minX(0), minY(0), maxX(0), maxY(0) {}
    BoundingBox(double minX, double minY, double maxX, double maxY)
        : minX(minX), minY(minY), maxX(maxX), maxY(maxY) {}
};

struct Config {
    // Input/Output
    std::string inputFile;
    std::string outputFile;
    std::string outputFormat = "GTiff";
    
    // Transformation options
    std::optional<std::string> targetCRS;
    std::optional<BoundingBox> clipBounds;
    bool applyNodataMask = false;
    double nodataValue = -9999.0;
    
    // Processing options
    bool verbose = false;
    int compressionLevel = 6;
    
    // Validation
    bool isValid() const;
    void validate() const;
};

class ConfigLoader {
public:
    static Config loadFromJson(const std::string& configPath, int index = 0);
    static Config loadFromArgs(int argc, char* argv[]);
    static std::vector<Config> loadAllFromJson(const std::string& configPath);
    static Config parseConfigFromJson(const json& configJson);
private:
    static void printUsage(const char* programName);
    //static Config parseConfigFromJson(const json& configJson);

};

} // namespace geo