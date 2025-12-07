#include "cdm_parser.h"
#include <iostream>
#include <sstream>
#include <iomanip>

static CDMTransactionProcessor cdmProcessor;

CDMTransactionProcessor::CDMTransactionProcessor() {
    dispensePattern = std::regex(R"(WFS_CMD_CDM_DISPENSE.*ReqID:\s*(\d+)(?:\s+(\w+))?)");
    presentPattern = std::regex(R"(WFS_CMD_CDM_PRESENT.*ReqID:\s*(\d+)(?:\s+(\w+))?)");
    errorPattern = std::regex(R"(DSP--(.+))");
    completePattern = std::regex(R"(WFS_EXECUTE_COMPLETE.*?(\d+)\s+EventID:\s*(302|303)\s+HR:\s*(.+))");
    timestampPattern = std::regex(R"(^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3})");
    mStatusPattern = std::regex(R"(MStatus\s+(\d+))");
}

std::string CDMTransactionProcessor::extractTimestamp(const std::string& line) {
    std::smatch match;
    if (std::regex_search(line, match, timestampPattern)) {
        return match.str(0);
    }
    return "";
}

std::string CDMTransactionProcessor::extractReqId(const std::string& line) {
    std::smatch match;

    if (std::regex_search(line, match, dispensePattern)) {
        return match.str(1);
    }

    if (std::regex_search(line, match, presentPattern)) {
        return match.str(1);
    }

    return "";
}

std::string CDMTransactionProcessor::extractErrorMessage(const std::string& line) {
    std::smatch match;
    if (std::regex_search(line, match, errorPattern)) {
        std::string errorMsg = match.str(1);
        size_t end = errorMsg.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            errorMsg = errorMsg.substr(0, end + 1);
        }
        return "DSP--" + errorMsg;
    }
    return "";
}

std::string CDMTransactionProcessor::extractMStatus(const std::string& line) {
    std::smatch match;
    if (std::regex_search(line, match, mStatusPattern)) {
        return match.str(1);
    }
    return "";
}

std::string CDMTransactionProcessor::extractCompleteReqId(const std::string& line) {
    std::smatch match;
    if (std::regex_search(line, match, completePattern)) {
        return match.str(1);
    }
    return "";
}

std::string CDMTransactionProcessor::extractResult(const std::string& line) {
    std::smatch match;
    if (std::regex_search(line, match, completePattern)) {
        std::string hrSection = match.str(3);

        std::regex errorNamePattern(R"(\(([^)]+)\))");
        std::smatch errorMatch;
        if (std::regex_search(hrSection, errorMatch, errorNamePattern)) {
            return errorMatch.str(1);
        }

        if (hrSection.find("0") != std::string::npos && hrSection.find("-") == std::string::npos) {
            return "WFS_SUCCESS";
        }
        else if (hrSection.find("-302") != std::string::npos) {
            return "WFS_ERR_CDM_CASHUNITERROR";
        }
        else if (hrSection.find("-306") != std::string::npos) {
            return "WFS_ERR_CDM_NOTDISPENSABLE";
        }
        else if (hrSection.find("-320") != std::string::npos) {
            return "WFS_ERR_CDM_PRERRORNOITEMS";
        }
        else if (hrSection.find("-322") != std::string::npos) {
            return "WFS_ERR_CDM_PRERRORUNKNOWN";
        }
        else if (hrSection.find("-14") != std::string::npos) {
            return "WFS_ERR_HARDWARE_ERROR";
        }
        else {
            return hrSection;
        }
    }
    return "";
}

bool CDMTransactionProcessor::isDSPError(const std::string& line) {
    if (line.find("[DEBUG:20]") != std::string::npos) {
        return false;
    }
    return line.find("DSP--") != std::string::npos;
}

std::string CDMTransaction::getFormattedRequest() const {
    return request.timestamp + "    " + request.command + " ReqID: " + request.reqId;
}

std::string CDMTransaction::getFormattedError() const {
    if (!request.hasError) return "";

    std::string result = request.timestamp + "    MStatus " + request.mStatus + " " + request.errorMessage;
    return result;
}

std::string CDMTransaction::getFormattedResponse() const {
    return response.timestamp + "    " + response.result + " ResID: " + response.reqId;
}

std::string CDMTransactionProcessor::processLine(const std::string& line) {
    std::string timestamp = extractTimestamp(line);
    std::string result = "";

    // Check for CDM commands (DISPENSE or PRESENT)
    if (line.find("WFS_CMD_CDM_DISPENSE") != std::string::npos ||
        line.find("WFS_CMD_CDM_PRESENT") != std::string::npos) {

        std::string reqId = extractReqId(line);
        if (!reqId.empty()) {
            CDMTransaction transaction;
            transaction.request.reqId = reqId;
            transaction.request.timestamp = timestamp;

            std::regex spPattern(R"(ReqID:\s*\d+\s+(\w+))");
            std::smatch match;
            std::string serviceProvider = "";
            if (std::regex_search(line, match, spPattern)) {
                serviceProvider = " (" + match.str(1) + ")";
            }

            if (line.find("WFS_CMD_CDM_DISPENSE") != std::string::npos) {
                transaction.request.command = "WFS_CMD_CDM_DISPENSE";
            }
            else {
                transaction.request.command = "WFS_CMD_CDM_PRESENT";
            }

            pendingTransactions[reqId] = transaction;

            result = timestamp + "    " + transaction.request.command + " ReqID: " + reqId + serviceProvider;
        }
    }

    else if (isDSPError(line) && !pendingTransactions.empty()) {
        std::string errorMsg = extractErrorMessage(line);
        std::string mStatus = extractMStatus(line);

        if (!errorMsg.empty()) {
            for (auto& pair : pendingTransactions) {
                if (!pair.second.request.hasError) {
                    pair.second.request.hasError = true;
                    pair.second.request.errorMessage = errorMsg;
                    pair.second.request.mStatus = mStatus.empty() ? "0" : mStatus;

                    result = pair.second.getFormattedError();
                    break;
                }
            }
        }
    }

    else if (line.find("WFS_EXECUTE_COMPLETE") != std::string::npos) {
        std::string reqId = extractCompleteReqId(line);
        std::string resultCode = extractResult(line);

        if (!reqId.empty() && !resultCode.empty()) {
            auto it = pendingTransactions.find(reqId);
            if (it != pendingTransactions.end()) {
                it->second.response.reqId = reqId;
                it->second.response.result = resultCode;
                it->second.response.timestamp = timestamp;
                it->second.isComplete = true;

                result = it->second.getFormattedResponse();
                pendingTransactions.erase(it);
            }
        }
    }

    return result;
}

void CDMTransactionProcessor::clear() {
    pendingTransactions.clear();
}

bool CDMTransactionProcessor::isCDMData(const std::string& line) {
    if (line.find("[DEBUG:20]") != std::string::npos) {
        return false;
    }

    if (line.find("WFS_GETINFO_COMPLETE") != std::string::npos) {
        return false;
    }

    // Only consider it CDM data if:
    // 1. It's a CDM command (DISPENSE or PRESENT)
    // 2. It's a WFS_EXECUTE_COMPLETE with EXACTLY EventID 302 or 303
    // 3. It's a DSP-- error (will be filtered later based on pending transactions)

    if (line.find("WFS_CMD_CDM_DISPENSE") != std::string::npos ||
        line.find("WFS_CMD_CDM_PRESENT") != std::string::npos) {
        return true;
    }

    // Only WFS_EXECUTE_COMPLETE with EventID 302 or 303
    if (line.find("WFS_EXECUTE_COMPLETE") != std::string::npos) {
        if (line.find("EventID: 302") != std::string::npos ||
            line.find("EventID: 303") != std::string::npos) {
            return true;
        }
        return false;
    }

    if (line.find("DSP--") != std::string::npos) {
        return true;
    }

    return false;
}

std::string CDMTransactionProcessor::processCDMLine(const std::string& line) {
    return cdmProcessor.processLine(line);
}