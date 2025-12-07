using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using JalinTools.Models;

namespace JalinTools.Services
{
    /// <summary>
    /// Process CDM transactions to match C++ implementation
    /// </summary>
    public class CDMProcessor
    {
        private readonly Dictionary<string, CDMTransaction> pendingTransactions = new Dictionary<string, CDMTransaction>();
        
        private readonly Regex dispensePattern = new Regex(@"WFS_CMD_CDM_DISPENSE.*ReqID:\s*(\d+)(?:\s+(\w+))?");
        private readonly Regex presentPattern = new Regex(@"WFS_CMD_CDM_PRESENT.*ReqID:\s*(\d+)(?:\s+(\w+))?");
        private readonly Regex completePattern = new Regex(@"WFS_EXECUTE_COMPLETE.*?(\d+)\s+EventID:\s*(302|303)\s+HR:\s*(.+)");
        private readonly Regex errorPattern = new Regex(@"DSP--(.+)");
        private readonly Regex mStatusPattern = new Regex(@"MStatus\s+(\d+)");
        private readonly Regex timestampPattern = new Regex(@"^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3}");

        public string ProcessLine(string line)
        {
            string timestamp = ExtractTimestamp(line);
            
            // Check for CDM commands (DISPENSE or PRESENT)
            if (line.Contains("WFS_CMD_CDM_DISPENSE") || line.Contains("WFS_CMD_CDM_PRESENT"))
            {
                return ProcessCDMCommand(line, timestamp);
            }
            
            // Check for DSP errors
            if (IsDSPError(line) && pendingTransactions.Count > 0)
            {
                return ProcessDSPError(line, timestamp);
            }
            
            // Check for WFS_EXECUTE_COMPLETE
            if (line.Contains("WFS_EXECUTE_COMPLETE"))
            {
                return ProcessComplete(line, timestamp);
            }
            
            return string.Empty;
        }

        private string ProcessCDMCommand(string line, string timestamp)
        {
            Regex pattern = line.Contains("DISPENSE") ? dispensePattern : presentPattern;
            var match = pattern.Match(line);
            
            if (match.Success)
            {
                string reqId = match.Groups[1].Value;
                string serviceProvider = match.Groups.Count > 2 ? match.Groups[2].Value : string.Empty;
                
                var transaction = new CDMTransaction
                {
                    Request = new CDMRequest
                    {
                        ReqId = reqId,
                        Timestamp = timestamp,
                        Command = line.Contains("DISPENSE") ? "WFS_CMD_CDM_DISPENSE" : "WFS_CMD_CDM_PRESENT",
                        ServiceProvider = serviceProvider
                    }
                };
                
                pendingTransactions[reqId] = transaction;
                
                string sp = string.IsNullOrEmpty(serviceProvider) ? "" : $" ({serviceProvider})";
                return $"{timestamp}    {transaction.Request.Command} ReqID: {reqId}{sp}";
            }
            
            return string.Empty;
        }

        private string ProcessDSPError(string line, string timestamp)
        {
            var errorMatch = errorPattern.Match(line);
            var mStatusMatch = mStatusPattern.Match(line);
            
            if (errorMatch.Success)
            {
                string errorMsg = "DSP--" + errorMatch.Groups[1].Value.TrimEnd();
                string mStatus = mStatusMatch.Success ? mStatusMatch.Groups[1].Value : "0";
                
                foreach (var pair in pendingTransactions.Values)
                {
                    if (!pair.Request.HasError)
                    {
                        pair.Request.HasError = true;
                        pair.Request.ErrorMessage = errorMsg;
                        pair.Request.MStatus = mStatus;
                        
                        return $"{pair.Request.Timestamp}    MStatus {mStatus} {errorMsg}";
                    }
                }
            }
            
            return string.Empty;
        }

        private string ProcessComplete(string line, string timestamp)
        {
            var match = completePattern.Match(line);
            
            if (match.Success)
            {
                string reqId = match.Groups[1].Value;
                string hrSection = match.Groups[3].Value;
                string result = ExtractResult(hrSection);
                
                if (pendingTransactions.TryGetValue(reqId, out var transaction))
                {
                    transaction.Response.ReqId = reqId;
                    transaction.Response.Result = result;
                    transaction.Response.Timestamp = timestamp;
                    transaction.IsComplete = true;
                    
                    string output = $"{timestamp}    {result} ResID: {reqId}";
                    pendingTransactions.Remove(reqId);
                    
                    return output;
                }
            }
            
            return string.Empty;
        }

        private string ExtractResult(string hrSection)
        {
            // Extract result from parentheses
            var match = Regex.Match(hrSection, @"\(([^)]+)\)");
            if (match.Success)
            {
                return match.Groups[1].Value;
            }
            
            // Check for success
            if (hrSection.Contains("0") && !hrSection.Contains("-"))
            {
                return "WFS_SUCCESS";
            }
            
            // Map error codes
            if (hrSection.Contains("-302")) return "WFS_ERR_CDM_CASHUNITERROR";
            if (hrSection.Contains("-306")) return "WFS_ERR_CDM_NOTDISPENSABLE";
            if (hrSection.Contains("-320")) return "WFS_ERR_CDM_PRERRORNOITEMS";
            if (hrSection.Contains("-322")) return "WFS_ERR_CDM_PRERRORUNKNOWN";
            if (hrSection.Contains("-14")) return "WFS_ERR_HARDWARE_ERROR";
            
            return hrSection;
        }

        private string ExtractTimestamp(string line)
        {
            var match = timestampPattern.Match(line);
            return match.Success ? match.Value : string.Empty;
        }

        private bool IsDSPError(string line)
        {
            if (line.Contains("[DEBUG:20]")) return false;
            return line.Contains("DSP--");
        }

        public bool IsCDMData(string line)
        {
            if (line.Contains("[DEBUG:20]")) return false;
            if (line.Contains("WFS_GETINFO_COMPLETE")) return false;
            
            if (line.Contains("WFS_CMD_CDM_DISPENSE") || line.Contains("WFS_CMD_CDM_PRESENT"))
                return true;
            
            if (line.Contains("WFS_EXECUTE_COMPLETE"))
            {
                return line.Contains("EventID: 302") || line.Contains("EventID: 303");
            }
            
            if (line.Contains("DSP--"))
                return true;
            
            return false;
        }

        public void Clear()
        {
            pendingTransactions.Clear();
        }
    }
}
