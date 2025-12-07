#pragma once
#include <string>
#include <vector>
#include <map>
#include <regex>

struct CDMRequest {
    std::string reqId;
    std::string command;
    std::string timestamp;
    std::string errorMessage;
    std::string mStatus;
    bool hasError = false;
};

struct CDMResponse {
    std::string reqId;
    std::string result;
    std::string timestamp;
};

struct CDMTransaction {
    CDMRequest request;
    CDMResponse response;
    bool isComplete = false;
    std::string getFormattedRequest() const;
    std::string getFormattedError() const;
    std::string getFormattedResponse() const;
};

class CDMTransactionProcessor {
private:
    std::map<std::string, CDMTransaction> pendingTransactions;

    std::regex dispensePattern;
    std::regex presentPattern;
    std::regex errorPattern;
    std::regex completePattern;
    std::regex timestampPattern;
    std::regex mStatusPattern;

    std::string extractTimestamp(const std::string& line);
    std::string extractReqId(const std::string& line);
    std::string extractErrorMessage(const std::string& line);
    std::string extractMStatus(const std::string& line);
    std::string extractResult(const std::string& line);
    std::string extractCompleteReqId(const std::string& line);
    bool isDSPError(const std::string& line);

public:
    CDMTransactionProcessor();
    std::string processLine(const std::string& line);
    void clear();

    static bool isCDMData(const std::string& line);
    static std::string processCDMLine(const std::string& line);
};