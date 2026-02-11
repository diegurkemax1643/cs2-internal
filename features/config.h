#pragma once
#include <string>
#include <vector>

namespace Config {
    bool Save(const std::string& filename = "config.json");
    bool Load(const std::string& filename = "config.json");
    std::string GetConfigPath(const std::string& filename = "config.json");
    std::vector<std::string> GetAvailableConfigs();
    bool DeleteConfig(const std::string& filename);
}

