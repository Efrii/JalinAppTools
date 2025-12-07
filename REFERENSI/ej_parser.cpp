#include "ej_parser.h"
#include "cassette_detector.h"
#include "cdm_parser.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <numeric>
#include <unordered_set>

// Keyword patterns from reference config.ini (line 62)
static std::vector<std::string> logKeywordPatterns = {
    "CARD INSERTED",
    "\\[INFO:10\\]",
    "\\[PRINT:10\\]",
    "\\[XML:15\\]",
    "WFS_CMD_CDM_DISPENSE",
    "WFS_CMD_CDM_PRESENT",
    "EventID: 302",
    "EventID: 303",
    "DSP--"
};

// Remove patterns from reference config.ini (line 65)
static std::vector<std::string> logRemovePatterns = {
    "\\[TID:0x[A-Fa-f0-9]+\\] \\[INFO:\\d+\\]",
    "\\[TID:0x[A-Fa-f0-9]+\\] \\[PRINT:\\d+\\]",
    "iforme:\\d+\\] Motorized card reader detected",
    "Motorized card reader detected",
    "\\[DEBUG:20\\]",
    "XFS WFSAsyncExecute.*CMD: 30[23].*ReqID",
    "\\(class\\w+\\.cpp:\\d+\\)"
};

// Transaction triggers from reference
static std::vector<std::string> transactionTriggers = {
    "CARD INSERTED",
    "CARD LESS SELECTED"
};

static std::vector<std::string> allTransactionTriggers = {
    "CARD INSERTED",
    "SUPERVISOR SAFE OPEN",
    "CARD LESS SELECTED"
};

EJParser::EJParser(const std::string& inputFile, const std::string& outputFile, EJParserFormat fmt)
    : inputFilePath(inputFile), outputFilePath(outputFile), format(fmt) {
    initializePatterns();
}

void EJParser::initializePatterns() {
    keywordPatterns = logKeywordPatterns;
    removePatterns = logRemovePatterns;
}

bool EJParser::loadConfig(const std::string& configPath) {
    return true;
}

void EJParser::setTID(const std::string& tidValue) {
    tid = tidValue;
}

void EJParser::setParticipantId(const std::string& participantIdValue) {
    participantId = participantIdValue;
}

// Matching reference isTransactionStart
bool EJParser::isTransactionStart(const std::string& line) {
    for (const auto& trigger : allTransactionTriggers) {
        if (line.find(trigger) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Matching reference getTransactionSeparator
std::string EJParser::getTransactionSeparator(const std::string& line) {
    std::ostringstream separator;

    separator << "\n\n";
    separator << "================================================================================\n";

    std::string transactionType = "UNKNOWN TRANSACTION";
    for (const auto& trigger : transactionTriggers) {
        if (line.find(trigger) != std::string::npos) {
            transactionType = trigger;
            break;
        }
    }

    separator << "                           " << transactionType << " TRANSACTION\n";
    separator << "================================================================================\n";

    return separator.str();
}

std::string EJParser::cleanTimestamp(const std::string& line) {
    std::string filteredLine = line;
    
    // Apply remove patterns (matching reference)
    for (const auto& pattern : removePatterns) {
        try {
            filteredLine = std::regex_replace(filteredLine, std::regex(pattern), "");
        } catch (...) {
        }
    }

    // Matching reference lines 238-246
    std::smatch match;
    if (std::regex_search(line, match, std::regex(R"(^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3})"))) {
        std::string timestamp = match.str(0);
        std::string remainingContent = filteredLine.substr(timestamp.length());

        remainingContent = std::regex_replace(remainingContent, std::regex(R"(^\s*\[TID:0x[A-Fa-f0-9]+\]\s*)"), "");
        remainingContent = std::regex_replace(remainingContent, std::regex(R"(^\s*\[[A-Z]+:\d+\]\s*)"), "");
        remainingContent = std::regex_replace(remainingContent, std::regex(R"(^\s+)"), "");

        filteredLine = timestamp + "    " + remainingContent;
    }

    return filteredLine;
}

bool EJParser::shouldIncludeLine(const std::string& line) {
    // Matching reference lines 200-204
    if (line.find("Motorized card reader detected") != std::string::npos) return false;
    if (line.find("[DEBUG:20]") != std::string::npos) return false;
    if (line.find("WFS_GETINFO_COMPLETE") != std::string::npos) return false;

    // Matching reference line 206 - regex match
    std::string keywordRegexStr = "(" + std::accumulate(keywordPatterns.begin(), keywordPatterns.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a.empty() ? b : a + "|" + b; }) + ")";
    
    try {
        std::regex keywordPattern(keywordRegexStr);
        return std::regex_search(line, keywordPattern);
    } catch (...) {
        for (const auto& keyword : keywordPatterns) {
            if (line.find(keyword) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
}

std::string EJParser::filterLine(const std::string& line) {
    return cleanTimestamp(line);
}

// Extract RRN from line containing "RRN" keyword
std::string EJParser::extractRRN(const std::string& line) {
    size_t rrnPos = line.find("RRN");
    if (rrnPos != std::string::npos) {
        // Find the number after "RRN"
        size_t start = rrnPos + 3; // Skip "RRN"
        while (start < line.length() && std::isspace(line[start])) start++;
        
        size_t end = start;
        while (end < line.length() && std::isdigit(line[end])) end++;
        
        if (end > start) {
            return line.substr(start, end - start);
        }
    }
    return "";
}

// Extract transaction time from line (timestamp from line containing RRN)
std::string EJParser::extractTransactionTime(const std::string& line) {
    // Extract timestamp from line format: DD.MM.YYYY HH:MM:SS.mmm
    std::smatch match;
    if (std::regex_search(line, match, std::regex(R"((\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}))"))) {
        return match.str(1);
    }
    return "";
}

// Extract journal time from log line
std::string EJParser::extractJournalTime(const std::string& line) {
    // Same as transaction time - extract timestamp
    return extractTransactionTime(line);
}

// Process file with Standard format (CSV output)
bool EJParser::processFileStandard() {
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        lastError = "Failed to open input file: " + inputFilePath;
        return false;
    }

    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        lastError = "Failed to create output file: " + outputFilePath;
        inputFile.close();
        return false;
    }

    // Write CSV header
    outputFile << "TID|Trace Number|Participant ID|Transaction Time|Journal Time|text\n";

    std::string line;
    std::vector<std::pair<std::string, std::string>> bufferedLines; // (journalTime, content)
    std::string currentTraceNumber;
    std::string currentTransactionTime;
    bool skipSupervisorSession = false;
    bool inTransaction = false;

    while (std::getline(inputFile, line)) {
        // Remove carriage return
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Skip supervisor sessions
        if (line.find("SUPERVISOR SAFE OPEN") != std::string::npos) {
            skipSupervisorSession = true;
            continue;
        }

        if (skipSupervisorSession) {
            if (line.find("CARD INSERTED") != std::string::npos ||
                line.find("CARD LESS SELECTED") != std::string::npos) {
                skipSupervisorSession = false;
            }
            else {
                continue;
            }
        }

        // Skip debug and specific lines
        if (line.find("Motorized card reader detected") != std::string::npos) continue;
        if (line.find("[DEBUG:20]") != std::string::npos) continue;
        if (line.find("WFS_GETINFO_COMPLETE") != std::string::npos) continue;

        // Check for CARD INSERTED - start of new transaction
        if (line.find("CARD INSERTED") != std::string::npos) {
            inTransaction = true;
            bufferedLines.clear();
        }

        // Extract RRN and set as transaction time
        std::string rrn = extractRRN(line);
        if (!rrn.empty()) {
            currentTraceNumber = rrn;
            currentTransactionTime = extractTransactionTime(line);
            
            // Write all buffered lines with this RRN and transaction time
            for (const auto& buffered : bufferedLines) {
                outputFile << tid << "|"
                          << currentTraceNumber << "|"
                          << participantId << "|"
                          << currentTransactionTime << "|"
                          << buffered.first << "|"
                          << buffered.second << "\n";
            }
            bufferedLines.clear();
        }

        // Filter only [INFO:10] and [PRINT:10] lines for Standard format
        if (line.find("[INFO:10]") != std::string::npos || line.find("[PRINT:10]") != std::string::npos) {
            // Extract journal time
            std::string journalTime = extractJournalTime(line);

            // Clean the line - apply removePatterns
            std::string cleanedLine = line;
            for (const auto& pattern : removePatterns) {
                try {
                    cleanedLine = std::regex_replace(cleanedLine, std::regex(pattern), "");
                } catch (...) {}
            }

            // Extract only the text content (after timestamp and tags)
            std::smatch match;
            if (std::regex_search(cleanedLine, match, std::regex(R"(^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3})"))) {
                std::string timestamp = match.str(0);
                std::string content = cleanedLine.substr(timestamp.length());
                
                // Remove leading/trailing whitespace
                size_t start = content.find_first_not_of(" \t\r\n");
                size_t end = content.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    content = content.substr(start, end - start + 1);
                }

                // If we have RRN, write immediately; otherwise buffer
                if (!currentTraceNumber.empty() && !currentTransactionTime.empty()) {
                    outputFile << tid << "|"
                              << currentTraceNumber << "|"
                              << participantId << "|"
                              << currentTransactionTime << "|"
                              << journalTime << "|"
                              << content << "\n";
                } else if (inTransaction) {
                    // Buffer until we get RRN
                    bufferedLines.push_back({journalTime, content});
                }
            }
        }
    }

    inputFile.close();
    outputFile.close();
    return true;
}

// Process file with Lengkap format (existing implementation)
bool EJParser::processFileLengkap() {
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        lastError = "Failed to open input file: " + inputFilePath;
        return false;
    }

    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        lastError = "Failed to create output file: " + outputFilePath;
        inputFile.close();
        return false;
    }

    std::unordered_set<std::string> existingLines;

    std::string line;
    bool skipSupervisorSession = false;

    while (std::getline(inputFile, line)) {
        // Matching reference line 181-183
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Matching reference lines 185-198
        if (line.find("SUPERVISOR SAFE OPEN") != std::string::npos) {
            skipSupervisorSession = true;
            continue;
        }

        if (skipSupervisorSession) {
            if (line.find("CARD INSERTED") != std::string::npos ||
                line.find("CARD LESS SELECTED") != std::string::npos) {
                skipSupervisorSession = false;
            }
            else {
                continue;
            }
        }

        // Matching reference lines 200-204
        if (line.find("Motorized card reader detected") != std::string::npos) continue;
        if (line.find("[DEBUG:20]") != std::string::npos) continue;
        if (line.find("WFS_GETINFO_COMPLETE") != std::string::npos) continue;

        // Matching reference line 206
        if (shouldIncludeLine(line)) {
            // Matching reference lines 207-218 - Cassette
            if (CassetteProcessor::isCassetteData(line)) {
                if (existingLines.find(line) == existingLines.end()) {
                    std::string cassetteReport = CassetteProcessor::processCassetteXML(line);
                    if (!cassetteReport.empty()) {
                        outputFile << cassetteReport << "\n";
                        outputFile.flush();
                        existingLines.insert(line);
                    }
                }
                continue;
            }

            // Matching reference lines 220-230 - CDM
            if (CDMTransactionProcessor::isCDMData(line)) {
                std::string cdmOutput = CDMTransactionProcessor::processCDMLine(line);
                if (!cdmOutput.empty()) {
                    if (existingLines.find(cdmOutput) == existingLines.end()) {
                        outputFile << cdmOutput << "\n";
                        outputFile.flush();
                        existingLines.insert(cdmOutput);
                    }
                }
                continue;
            }

            // Matching reference lines 232-247 - Filter
            std::string filteredLine = filterLine(line);

            // Matching reference lines 249-258
            if (existingLines.find(filteredLine) == existingLines.end()) {
                if (isTransactionStart(filteredLine)) {
                    std::string separator = getTransactionSeparator(filteredLine);
                    outputFile << separator;
                    outputFile.flush();
                }

                outputFile << filteredLine << "\n";
                outputFile.flush();
                existingLines.insert(filteredLine);
            }
        }
    }

    inputFile.close();
    outputFile.close();

    return true;
}

bool EJParser::processFile() {
    // Route to appropriate processor based on format
    if (format == EJParserFormat::Standard) {
        return processFileStandard();
    } else {
        return processFileLengkap();
    }
}

std::string EJParser::getLastError() const {
    return lastError;
}
