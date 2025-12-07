#pragma once
#include <string>
#include <vector>

// EJ Parser format options
enum class EJParserFormat {
    Standard,
    Lengkap
};

class EJParser {
private:
    std::string inputFilePath;
    std::string outputFilePath;
    EJParserFormat format;
    
    // User inputs for Standard format
    std::string tid;
    std::string participantId;

    // Keyword patterns for filtering
    std::vector<std::string> keywordPatterns;
    std::vector<std::string> removePatterns;
    std::vector<std::string> transactionTriggers;

    // Helper methods
    void initializePatterns();
    bool isTransactionStart(const std::string& line);
    std::string getTransactionSeparator(const std::string& line);
    std::string filterLine(const std::string& line);
    std::string cleanTimestamp(const std::string& line);
    bool shouldIncludeLine(const std::string& line);
    
    // Standard format helpers
    bool processFileStandard();
    bool processFileLengkap();
    std::string extractRRN(const std::string& line);
    std::string extractTransactionTime(const std::string& line);
    std::string extractJournalTime(const std::string& line);

public:
    EJParser(const std::string& inputFile, const std::string& outputFile, EJParserFormat fmt);
    
    // Setters for Standard format
    void setTID(const std::string& tidValue);
    void setParticipantId(const std::string& participantIdValue);
    
    // Load patterns from config file
    bool loadConfig(const std::string& configPath);
    
    // Main processing method
    bool processFile();
    
    // Get last error message
    std::string getLastError() const;

private:
    std::string lastError;
};
