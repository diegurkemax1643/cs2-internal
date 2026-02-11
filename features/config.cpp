#include "config.h"
#include "esp.h"
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <Shlobj.h>

std::string Config::GetConfigPath(const std::string& filename) {
    char path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) == S_OK) {
        std::string configDir = std::string(path) + "\\CS2Internal";
        CreateDirectoryA(configDir.c_str(), NULL);
        return configDir + "\\" + filename;
    }
    // Fallback to current directory
    return filename;
}

bool Config::Save(const std::string& filename) {
    std::string filepath = GetConfigPath(filename);
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Write ESP settings
    file << "{\n";
    file << "  \"esp\": {\n";
    file << "    \"enabled\": " << (ESP::enabled ? "true" : "false") << ",\n";
    file << "    \"boxEnabled\": " << (ESP::boxEnabled ? "true" : "false") << ",\n";
    file << "    \"corneredBoxEnabled\": " << (ESP::corneredBoxEnabled ? "true" : "false") << ",\n";
    file << "    \"boxFillEnabled\": " << (ESP::boxFillEnabled ? "true" : "false") << ",\n";
    file << "    \"glowEnabled\": " << (ESP::glowEnabled ? "true" : "false") << ",\n";
    file << "    \"healthBarEnabled\": " << (ESP::healthBarEnabled ? "true" : "false") << ",\n";
    file << "    \"nameEnabled\": " << (ESP::nameEnabled ? "true" : "false") << ",\n";
    file << "    \"distanceEnabled\": " << (ESP::distanceEnabled ? "true" : "false") << ",\n";
    file << "    \"weaponEnabled\": " << (ESP::weaponEnabled ? "true" : "false") << ",\n";
    file << "    \"headCircleEnabled\": " << (ESP::headCircleEnabled ? "true" : "false") << ",\n";
    file << "    \"linesEnabled\": " << (ESP::linesEnabled ? "true" : "false") << ",\n";
    file << "    \"viewEspEnabled\": " << (ESP::viewEspEnabled ? "true" : "false") << ",\n";
    file << "    \"linesOrigin\": " << ESP::linesOrigin << ",\n";
    file << "    \"teamEspEnabled\": " << (ESP::teamEspEnabled ? "true" : "false") << ",\n";
    file << "    \"chamsEnabled\": " << (ESP::chamsEnabled ? "true" : "false") << ",\n";
    file << "    \"enemyBoxColor\": [" << ESP::enemyBoxColor.x << ", " << ESP::enemyBoxColor.y << ", " << ESP::enemyBoxColor.z << ", " << ESP::enemyBoxColor.w << "],\n";
    file << "    \"teamBoxColor\": [" << ESP::teamBoxColor.x << ", " << ESP::teamBoxColor.y << ", " << ESP::teamBoxColor.z << ", " << ESP::teamBoxColor.w << "],\n";
    file << "    \"enemyGlowColor\": [" << ESP::enemyGlowColor.x << ", " << ESP::enemyGlowColor.y << ", " << ESP::enemyGlowColor.z << ", " << ESP::enemyGlowColor.w << "],\n";
    file << "    \"teamGlowColor\": [" << ESP::teamGlowColor.x << ", " << ESP::teamGlowColor.y << ", " << ESP::teamGlowColor.z << ", " << ESP::teamGlowColor.w << "],\n";
    file << "    \"healthBarColor\": [" << ESP::healthBarColor.x << ", " << ESP::healthBarColor.y << ", " << ESP::healthBarColor.z << ", " << ESP::healthBarColor.w << "],\n";
    file << "    \"nameColor\": [" << ESP::nameColor.x << ", " << ESP::nameColor.y << ", " << ESP::nameColor.z << ", " << ESP::nameColor.w << "],\n";
    file << "    \"linesColor\": [" << ESP::linesColor.x << ", " << ESP::linesColor.y << ", " << ESP::linesColor.z << ", " << ESP::linesColor.w << "],\n";
    file << "    \"enemyChamsColor\": [" << ESP::enemyChamsColor.x << ", " << ESP::enemyChamsColor.y << ", " << ESP::enemyChamsColor.z << ", " << ESP::enemyChamsColor.w << "],\n";
    file << "    \"teamChamsColor\": [" << ESP::teamChamsColor.x << ", " << ESP::teamChamsColor.y << ", " << ESP::teamChamsColor.z << ", " << ESP::teamChamsColor.w << "]\n";
    file << "  }\n";
    file << "}\n";
    
    file.close();
    return true;
}

bool Config::Load(const std::string& filename) {
    std::string filepath = GetConfigPath(filename);
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
    }
    file.close();
    
    // Simple JSON parsing (basic implementation)
    // In a real scenario, you'd use a proper JSON library
    
    // Parse booleans
    size_t pos = content.find("\"enabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::enabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"boxEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::boxEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"corneredBoxEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::corneredBoxEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"boxFillEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::boxFillEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"glowEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::glowEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"healthBarEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::healthBarEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"nameEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::nameEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"distanceEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::distanceEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"weaponEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::weaponEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"headCircleEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::headCircleEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"linesEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::linesEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"viewEspEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::viewEspEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"teamEspEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::teamEspEnabled = (content[valPos] == 't');
        }
    }
    
    pos = content.find("\"chamsEnabled\":");
    if (pos != std::string::npos) {
        size_t valPos = content.find_first_of("tf", pos);
        if (valPos != std::string::npos) {
            ESP::chamsEnabled = (content[valPos] == 't');
        }
    }
    
    // Parse linesOrigin (integer)
    pos = content.find("\"linesOrigin\":");
    if (pos != std::string::npos) {
        size_t numStart = content.find_first_of("0123456789", pos);
        if (numStart != std::string::npos) {
            ESP::linesOrigin = std::stoi(content.substr(numStart));
        }
    }
    
    // Parse colors (arrays)
    auto parseColor = [&](const std::string& key, ImVec4& color) {
        pos = content.find("\"" + key + "\":");
        if (pos != std::string::npos) {
            size_t bracketStart = content.find('[', pos);
            size_t bracketEnd = content.find(']', bracketStart);
            if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                std::string colorStr = content.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                std::istringstream iss(colorStr);
                char comma;
                iss >> color.x >> comma >> color.y >> comma >> color.z >> comma >> color.w;
            }
        }
    };
    
    parseColor("enemyBoxColor", ESP::enemyBoxColor);
    parseColor("teamBoxColor", ESP::teamBoxColor);
    parseColor("enemyGlowColor", ESP::enemyGlowColor);
    parseColor("teamGlowColor", ESP::teamGlowColor);
    parseColor("healthBarColor", ESP::healthBarColor);
    parseColor("nameColor", ESP::nameColor);
    parseColor("linesColor", ESP::linesColor);
    parseColor("enemyChamsColor", ESP::enemyChamsColor);
    parseColor("teamChamsColor", ESP::teamChamsColor);
    
    return true;
}

std::vector<std::string> Config::GetAvailableConfigs() {
    std::vector<std::string> configs;
    std::string configDir = GetConfigPath("");
    // Remove filename to get directory
    size_t lastSlash = configDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        configDir = configDir.substr(0, lastSlash + 1);
    }
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((configDir + "*.json").c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findData.cFileName;
                // Remove .json extension for display
                if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json") {
                    configs.push_back(filename.substr(0, filename.size() - 5));
                } else {
                    configs.push_back(filename);
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    return configs;
}

bool Config::DeleteConfig(const std::string& filename) {
    std::string filepath = GetConfigPath(filename);
    // Ensure .json extension
    if (filepath.find(".json") == std::string::npos) {
        // Check if filename already has .json
        std::string justFilename = filename;
        size_t lastSlash = justFilename.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            justFilename = justFilename.substr(lastSlash + 1);
        }
        if (justFilename.find(".json") == std::string::npos) {
            filepath = GetConfigPath(filename + ".json");
        }
    }
    return DeleteFileA(filepath.c_str()) != 0;
}

