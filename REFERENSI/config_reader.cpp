#include "config_reader.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigReader::ConfigReader() {
}

std::string ConfigReader::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> ConfigReader::splitByComma(const std::string& value) const {
    std::vector<std::string> result;
    std::string current;
    bool inEscape = false;
    
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        
        if (c == '\\' && i + 1 < value.length()) {
            // Handle escape sequences
            current += c;
            current += value[i + 1];
            i++;
        }
        else if (c == ',') {
            std::string trimmed = trim(current);
            if (!trimmed.empty()) {
                result.push_back(trimmed);
            }
            current.clear();
        }
        else {
            current += c;
        }
    }
    
    // Add last item
    std::string trimmed = trim(current);
    if (!trimmed.empty()) {
        result.push_back(trimmed);
    }
    
    return result;
}

bool ConfigReader::loadFromFile(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }
        
        // Parse key=value
        size_t pos = trimmedLine.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(trimmedLine.substr(0, pos));
            std::string value = trim(trimmedLine.substr(pos + 1));
            configValues[key] = value;
        }
    }
    
    file.close();
    return true;
}

bool ConfigReader::loadFromString(const std::string& configContent) {
    std::istringstream stream(configContent);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Skip comments and empty lines
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }
        
        // Parse key=value
        size_t pos = trimmedLine.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(trimmedLine.substr(0, pos));
            std::string value = trim(trimmedLine.substr(pos + 1));
            configValues[key] = value;
        }
    }
    
    return true;
}

std::vector<std::string> ConfigReader::getKeywordPatterns() const {
    auto it = configValues.find("logKeywordPatterns");
    if (it != configValues.end()) {
        return splitByComma(it->second);
    }
    return std::vector<std::string>();
}

std::vector<std::string> ConfigReader::getRemovePatterns() const {
    auto it = configValues.find("logRemovePatterns");
    if (it != configValues.end()) {
        return splitByComma(it->second);
    }
    return std::vector<std::string>();
}
