#include "config/config.h"
#include "pipeline/pipeline.h"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        geo::Config config;
        
        // Check for help or no arguments
        if (argc < 2) {
            geo::ConfigLoader::printUsage(argv[0]);
            return 1;
        }
        
        try {
            std::cout << "Loading config from command line arguments..." << std::endl;
            config = geo::ConfigLoader::loadFromArgs(argc, argv);
            std::cout << "Config loaded successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Config error: " << e.what() << std::endl;
            geo::ConfigLoader::printUsage(argv[0]);
            return 1;
        }
        
        // Validate configuration
        try {
            config.validate();
            std::cout << "Config validation passed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Configuration validation failed: " << e.what() << std::endl;
            return 1;
        }
        
        if (!config.isValid()) {
            std::cerr << "Invalid configuration." << std::endl;
            return 1;
        }
        
        std::cout << "Creating pipeline..." << std::endl;
        
        // Create and run pipeline
        geo::Pipeline pipeline(config);
        if (pipeline.hasErrors()) {
            std::cerr << "Pipeline initialization failed: " << pipeline.getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "Running pipeline..." << std::endl;
        bool success = pipeline.run();
        if (!success) {
            std::cerr << "Pipeline execution failed: " << pipeline.getLastError() << std::endl;
            return 1;
        }
        
        if (config.verbose) {
            std::cout << "\nGeospatial processing completed successfully!" << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

// Example usage:
// ./your_program -i input.tif -o output.tif -f GTiff -v



