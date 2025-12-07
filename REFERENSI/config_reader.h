#pragma once
#include <string>
#include <vector>
#include <map>

class ConfigReader {
private:
    std::map<std::string, std::string> configValues;
    
public:
    ConfigReader();
    bool loadFromFile(const std::string& configPath);
    bool loadFromString(const std::string& configContent);
    
    std::vector<std::string> getKeywordPatterns() const;
    std::vector<std::string> getRemovePatterns() const;
    
private:
    std::vector<std::string> splitByComma(const std::string& value) const;
    std::string trim(const std::string& str) const;
};
