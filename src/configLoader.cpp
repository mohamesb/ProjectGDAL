#include "config.h"
#include "../include/json.hpp"  // nlohmann JSON
#include <fstream>
#include <iostream>
#include "configLoader.h"

using json = nlohmann::json;

Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    json j;
    file >> j;

    Config cfg;
    cfg.input_file = j.at("input_file").get<std::string>();
    cfg.output_file = j.at("output_file").get<std::string>();
    cfg.target_crs = j.at("target_crs").get<std::string>();
    cfg.clip_bounds = j.at("clip_bounds").get<std::vector<double>>();
    cfg.overwrite = j.value("overwrite", false); // optional

    return cfg;
}
