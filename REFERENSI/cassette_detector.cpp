#include "cassette_detector.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <iomanip>
#include <algorithm>
#include <ctime>

CassetteDetector::CassetteDetector(const std::string& data) : xmlData(data) {
    dispensedValues.resize(MAX_CASSETTES, 0);
    rejectedValues.resize(MAX_CASSETTES, 0);
    remainingValues.resize(MAX_CASSETTES, 0);
    denominations.resize(MAX_CASSETTES, 0);
    initialValues.resize(MAX_CASSETTES, 0);
    denominationsView.resize(MAX_CASSETTES, "");
    cassetteTypes.resize(MAX_CASSETTES, 0);
}

std::string CassetteDetector::formatCurrency(int value, const std::string& currency) {
    if (value == 0) return currency + " 0";

    std::ostringstream oss;
    oss << currency + " ";

    std::string numStr = std::to_string(value);
    std::string formatted;
    int count = 0;

    for (int i = static_cast<int>(numStr.length()) - 1; i >= 0; i--) {
        if (count > 0 && count % 3 == 0) {
            formatted = "." + formatted;
        }
        formatted = numStr[i] + formatted;
        count++;
    }

    oss << formatted;
    return oss.str();
}

std::string CassetteDetector::padRight(const std::string& str, int width) {
    if (static_cast<int>(str.length()) >= width) return str;
    return str + std::string(width - str.length(), ' ');
}

std::string CassetteDetector::padCenter(const std::string& str, int width) {
    if (static_cast<int>(str.length()) >= width) return str;
    int padding = width - static_cast<int>(str.length());
    int leftPad = padding / 2;
    int rightPad = padding - leftPad;
    return std::string(leftPad, ' ') + str + std::string(rightPad, ' ');
}

void CassetteDetector::parseXMLData() {
    std::regex timestampPattern(R"(^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3} \[TID:0x[0-9A-Fa-f]+\] \[XML:\d+\] )");
    std::string cleanedXML = std::regex_replace(xmlData, timestampPattern, "");

    std::regex attrPattern(R"((\w+)=([\w|]+))");
    cleanedXML = std::regex_replace(cleanedXML, attrPattern, "$1='$2'");

    parseXMLElements(cleanedXML);
}

void CassetteDetector::parseXMLElements(const std::string& xml) {
    std::regex cuPattern(R"(<cu\s+([^>]+)>)");
    std::sregex_iterator iter(xml.begin(), xml.end(), cuPattern);
    std::sregex_iterator end;

    int dcIndex = 1;

    for (; iter != end; ++iter) {
        std::string attributes = iter->str(1);
        std::map<std::string, std::string> attrs = parseAttributes(attributes);

        if (attrs.find("n") == attrs.end()) continue;

        int vIndex = std::stoi(attrs["n"]) - 1;
        if (vIndex < 0 || vIndex >= MAX_CASSETTES) continue;

        int vaIndex = std::stoi(attrs["v"]);
        int typIndex = std::stoi(attrs["typ"]);
        int denominationValue = std::stoi(attrs["v"]);
        std::string dcView = attrs["dc"];

        int parsedDcIndex = dcIndex;
        if (attrs["dc"] != "0" && !attrs["dc"].empty() && std::isdigit(attrs["dc"][0])) {
            parsedDcIndex = std::stoi(attrs["dc"]);
        }
        else {
            parsedDcIndex = dcIndex++;
        }

        if (cashTotalLabels.find(parsedDcIndex) == cashTotalLabels.end()) {
            if (typIndex == 2) {
                cashTotalLabels[parsedDcIndex] = "REJECT_" + dcView;
            }
            else if (typIndex == 3) {
                cashTotalLabels[parsedDcIndex] = "CASSETTE_" + dcView;
            }
            else if (typIndex == 6) {
                cashTotalLabels[parsedDcIndex] = "RETRACT_" + dcView;
            }
        }

        if (denominationValues.find(vaIndex) == denominationValues.end()) {
            denominationValues[vaIndex] = denominationValue;
        }

        initialValues[vIndex] = attrs.count("ic") ? std::stoi(attrs["ic"]) : 0;
        dispensedValues[vIndex] = attrs.count("ds") ? std::stoi(attrs["ds"]) : 0;
        rejectedValues[vIndex] = attrs.count("rj") ? std::stoi(attrs["rj"]) : 0;
        remainingValues[vIndex] = attrs.count("c") ? std::stoi(attrs["c"]) : 0;
        denominations[vIndex] = denominationValue / 100;
        denominationsView[vIndex] = formatCurrency((denominationValue / 100),"");
        cassetteTypes[vIndex] = typIndex;
    }
}

std::map<std::string, std::string> CassetteDetector::parseAttributes(const std::string& attrs) {
    std::map<std::string, std::string> result;
    std::regex attrRegex(R"((\w+)=['"](.*?)['"])");
    std::sregex_iterator iter(attrs.begin(), attrs.end(), attrRegex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        result[iter->str(1)] = iter->str(2);
    }

    return result;
}

std::string CassetteDetector::detectCassette() {
    parseXMLData();

    std::ostringstream sb;

    sb << "================================================================================\n";
    sb << "                          CASH DISPENSER STATUS REPORT\n";
    sb << "================================================================================\n";

    std::vector<int> activeCassettes;
    for (int i = 0; i < MAX_CASSETTES; i++) {
        if (initialValues[i] > 0 || dispensedValues[i] > 0 || rejectedValues[i] > 0 || remainingValues[i] > 0) {
            activeCassettes.push_back(i);
        }
    }

    std::vector<int> financialCassettes;
    for (int i : activeCassettes) {
        if (cassetteTypes[i] != 2 && cassetteTypes[i] != 6) {
            financialCassettes.push_back(i);
        }
    }

    if (activeCassettes.empty()) {
        sb << "No active cassettes detected.\n";
        sb << "================================================================================\n";
        return sb.str();
    }

    sb << "\nCASETTE SUMMARY:\n";
    sb << "--------------------------------------------------------------------------------\n";
    sb << "CASSETTE        DENOMINATION    INITIAL    DISPENSED   REJECTED   REMAINING\n";
    sb << "--------------------------------------------------------------------------------\n";

    for (int cassetteIndex : activeCassettes) {
        auto labelIt = std::find_if(cashTotalLabels.begin(), cashTotalLabels.end(),
            [cassetteIndex](const std::pair<int, std::string>& p) {
                return p.first == cassetteIndex + 1;
            });

        std::string cassetteLabel = (labelIt != cashTotalLabels.end()) ?
            labelIt->second : "CASSETTE_" + std::to_string(cassetteIndex + 1);

        sb << std::left << std::setw(16) << cassetteLabel.substr(0, 16)
            << std::setw(16) << denominationsView[cassetteIndex]
            << std::setw(11) << initialValues[cassetteIndex]
                << std::setw(12) << dispensedValues[cassetteIndex]
                    << std::setw(11) << rejectedValues[cassetteIndex]
                        << std::setw(10) << remainingValues[cassetteIndex] << "\n";
    }

    sb << "--------------------------------------------------------------------------------\n";

    int totalInitial = 0;
    int totalDispensed = 0;
    int totalRejected = 0;
    int totalRemaining = 0;
    bool hasDiscrepancy = false;

    sb << "\nSUMMARY PER DISPENSER CASSETTE:\n";
    sb << "--------------------------------------------------------------------------------\n";

    if (financialCassettes.empty()) {
        sb << "No dispenser cassettes found (only reject/retract cassettes detected).\n";
    }
    else {
        for (int cassetteIndex : financialCassettes) {
            auto labelIt = std::find_if(cashTotalLabels.begin(), cashTotalLabels.end(),
                [cassetteIndex](const std::pair<int, std::string>& p) {
                    return p.first == cassetteIndex + 1;
                });

            std::string cassetteLabel = (labelIt != cashTotalLabels.end()) ?
                labelIt->second : "CASSETTE_" + std::to_string(cassetteIndex + 1);

            int denomination = denominations[cassetteIndex];
            int initial = initialValues[cassetteIndex] * denomination;
            int dispensed = dispensedValues[cassetteIndex] * denomination;
            int rejected = rejectedValues[cassetteIndex] * denomination;
            int remaining = remainingValues[cassetteIndex] * denomination;
            int total = remaining + dispensed + rejected;

            std::string typeInfo = "";
            if (cassetteTypes[cassetteIndex] == 3) {
                typeInfo = " (DISPENSER)";
            }

            sb << cassetteLabel << typeInfo << ":\n";
            sb << "  Initial Amount   : " << formatCurrency(initial, "Rp") << "\n";
            sb << "  Dispensed Amount : " << formatCurrency(dispensed, "Rp") << "\n";
            sb << "  Rejected Amount  : " << formatCurrency(rejected, "Rp") << "\n";
            sb << "  Remaining Amount : " << formatCurrency(remaining, "Rp") << "\n";
            sb << "  Total Calculated : " << formatCurrency(total, "Rp") << "\n";

            if (initial > 0 && total != initial) {
                hasDiscrepancy = true;
                sb << "  \n*** SUSPECT JAMMED DETECTED ***\n";
                sb << "  TOTAL COUNTER NOT MATCH " << formatCurrency(total, "Rp")
                    << " WITH INITIAL COUNT " << formatCurrency(initial, "Rp")
                    << " ON " << cassetteLabel << "\n";
                sb << "  DIFFERENCE: " << formatCurrency(abs(total - initial), "Rp") << "\n";
            }

            totalInitial += initial;
            totalDispensed += dispensed;
            totalRejected += rejected;
            totalRemaining += remaining;
        }
    }

    std::vector<int> excludedCassettes;
    for (int i : activeCassettes) {
        if (cassetteTypes[i] == 2 || cassetteTypes[i] == 6) {
            excludedCassettes.push_back(i);
        }
    }

    if (denominationValues.size() > 1 && !financialCassettes.empty()) {
        sb << "\nSUMMARY BY DENOMINATION:\n";
        sb << "--------------------------------------------------------------------------------\n";

        for (const auto& kvp : denominationValues) {
            int denomination = kvp.second / 100;
            if (denomination == 0) continue;

            int denomInitial = 0, denomDispensed = 0, denomRejected = 0, denomRemaining = 0;

            for (int i = 0; i < MAX_CASSETTES; i++) {
                if (denominations[i] == denomination && cassetteTypes[i] != 2 && cassetteTypes[i] != 6) {
                    denomInitial += initialValues[i] * denomination;
                    denomDispensed += dispensedValues[i] * denomination;
                    denomRejected += rejectedValues[i] * denomination;
                    denomRemaining += remainingValues[i] * denomination;
                }
            }

            if (denomInitial > 0 || denomDispensed > 0 || denomRejected > 0 || denomRemaining > 0) {
                sb << "Denomination" << formatCurrency(denomination, "") << " :\n";
                sb << "  Initial   : " << formatCurrency(denomInitial, "Rp") << "\n";
                sb << "  Dispensed : " << formatCurrency(denomDispensed, "Rp") << "\n";
                sb << "  Rejected  : " << formatCurrency(denomRejected, "Rp") << "\n";
                sb << "  Remaining : " << formatCurrency(denomRemaining, "Rp") << "\n";

                int denomTotal = denomRemaining + denomDispensed + denomRejected;
                if (denomInitial > 0 && denomTotal != denomInitial) {
                    hasDiscrepancy = true;
                    sb << "  \n*** SUSPECT JAMMED DETECTED ***\n";
                    sb << "  TOTAL COUNTER NOT MATCH " << formatCurrency(denomTotal, "Rp")
                        << " WITH INITIAL COUNT " << formatCurrency(denomInitial, "Rp")
                        << " ON DENOMINATION " << formatCurrency(denomination, "") << "\n";
                }
            }
        }
    }

    if (!financialCassettes.empty()) {
        sb << "\nOVERALL CASH DISPENSER SUMMARY:\n";
        sb << "--------------------------------------------------------------------------------\n";
        sb << "Total Initial Amount    : " << formatCurrency(totalInitial, "Rp") << "\n";
        sb << "Total Dispensed Amount  : " << formatCurrency(totalDispensed, "Rp") << "\n";
        sb << "Total Rejected Amount   : " << formatCurrency(totalRejected, "Rp") << "\n";
        sb << "Total Remaining Amount  : " << formatCurrency(totalRemaining, "Rp") << "\n";
        sb << "Total in Dispenser      : " << formatCurrency(totalDispensed + totalRejected + totalRemaining, "Rp") << "\n";

        int overallTotal = totalDispensed + totalRejected + totalRemaining;
        if (totalInitial > 0 && overallTotal != totalInitial) {
            hasDiscrepancy = true;
            sb << "\n*** OVERALL DISCREPANCY DETECTED ***\n";
            sb << "Expected Total: " << formatCurrency(totalInitial, "Rp") << "\n";
            sb << "Actual Total:   " << formatCurrency(overallTotal, "Rp") << "\n";
            sb << "Difference:     " << formatCurrency(abs(overallTotal - totalInitial), "Rp") << "\n";
        }
    }

    if (!financialCassettes.empty()) {
        if (hasDiscrepancy) {
            sb << "\n*** ATTENTION: CASSETTE DISCREPANCIES DETECTED ***\n";
            sb << "Please check the dispenser for possible jamming or mechanical issues.\n";
        }
    }

    sb << "\n";
    sb << "================================================================================";

    return sb.str();
}

bool CassetteProcessor::isCassetteData(const std::string& line) {
    return line.find("[XML:15]") != std::string::npos &&
        line.find("<cunits") != std::string::npos;
}

std::string CassetteProcessor::processCassetteXML(const std::string& line) {
    if (!isCassetteData(line)) {
        return "";
    }
    CassetteDetector detector(line);
    std::string result = detector.detectCassette();

    std::regex lfRegex("\n");
    result = std::regex_replace(result, lfRegex, "\r\n");

    return result;
}