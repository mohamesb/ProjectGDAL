#pragma once
#include <string>
#include <vector>

struct Config {
    std::string input_file;
    std::string output_file;
    std::string target_crs;
    //std::vector<double> clip_bounds;  // [minX, minY, maxX, maxY]
    bool overwrite = false;
    // Optional bounds for clipping
    double clipMinX = -180.0;
    double clipMinY = -90.0;
    double clipMaxX = 180.0;
    double clipMaxY = 90.0;
    bool enableReproject = true;
    bool enableClip = false;
};
