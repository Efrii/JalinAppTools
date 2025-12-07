#pragma once
#include <string>
#include <vector>
#include <map>

class CassetteDetector {
private:
    std::string xmlData;
    std::map<int, std::string> cashTotalLabels;
    std::map<int, int> denominationValues;
    std::vector<int> dispensedValues;
    std::vector<int> rejectedValues;
    std::vector<int> remainingValues;
    std::vector<int> denominations;
    std::vector<int> initialValues;
    std::vector<std::string> denominationsView;
    std::vector<int> cassetteTypes;

    static const int MAX_CASSETTES = 5;
    static const int COLUMN_WIDTH = 30;

    std::string formatCurrency(int value, const std::string& currency);
    std::string padRight(const std::string& str, int width);
    std::string padCenter(const std::string& str, int width);
    void parseXMLData();
    void parseXMLElements(const std::string& xml);
    std::map<std::string, std::string> parseAttributes(const std::string& attrs);

public:
    CassetteDetector(const std::string& data);
    std::string detectCassette();
};

class CassetteProcessor {
public:
    static std::string processCassetteXML(const std::string& line);
    static bool isCassetteData(const std::string& line);
};
