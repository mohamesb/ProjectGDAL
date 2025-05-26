#pragma once
#include <string>
#include <vector>

struct Config {
    std::string input_file;
    std::string output_file;
    std::string target_crs;
    std::vector<double> clip_bounds;  // [minX, minY, maxX, maxY]
    bool overwrite = false;
};
