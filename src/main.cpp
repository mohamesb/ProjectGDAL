#include "config.h"
#include <iostream>
#include "configLoader.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./pipeline config.json" << std::endl;
        return 1;
    }

    try {
        Config config = load_config(argv[1]);
        std::cout << "Loaded config:" << std::endl;
        std::cout << "Input: " << config.input_file << std::endl;
        std::cout << "Output: " << config.output_file << std::endl;
        // Use config in your pipeline...
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}
