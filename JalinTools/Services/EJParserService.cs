using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using JalinTools.Helpers;
using JalinTools.Models;

namespace JalinTools.Services
{
    /// <summary>
    /// Core EJ Parser service with business logic
    /// </summary>
    public class EJParserService
    {
        private string lastError = string.Empty;

        public string GetLastError() => lastError;

        /// <summary>
        /// Main processing method - routes to appropriate format processor
        /// </summary>
        public async Task<bool> ProcessFileAsync(
            string inputPath,
            string outputPath,
            string tid,
            string participantId,
            EJParserFormat format)
        {
            try
            {
                if (format == EJParserFormat.Standard)
                {
                    return await ProcessStandardFormatAsync(inputPath, outputPath, tid, participantId);
                }
                else
                {
                    return await ProcessLengkapFormatAsync(inputPath, outputPath);
                }
            }
            catch (Exception ex)
            {
                lastError = ex.Message;
                return false;
            }
        }

        /// <summary>
        /// Process file with Standard format (TXT output with transaction buffering)
        /// </summary>
        private async Task<bool> ProcessStandardFormatAsync(
            string inputPath,
            string outputPath,
            string tid,
            string participantId)
        {
            try
            {
                var lines = await File.ReadAllLinesAsync(inputPath);
                var output = new StringBuilder();

                // Write TXT header
                output.AppendLine("TID|Trace Number|Participant ID|Transaction Time|Journal Time|text");

                var bufferedLines = new List<BufferedLine>();
                string currentTraceNumber = string.Empty;
                DateTime? currentTransactionTime = null;
                bool skipSupervisorSession = false;
                bool inTransaction = false;

                foreach (var line in lines)
                {
                    if (skipSupervisorSession)
                    {
                        if (line.Contains("CARD INSERTED") || line.Contains("CARD LESS SELECTED"))
                        {
                            skipSupervisorSession = false;
                        }
                        else
                        {
                            continue;
                        }
                    }

                    if (ShouldSkipLine(line))
                        continue;

                    // Check for transaction end without RRN - assign RRN 0
                    if ((line.Contains("TRANSACTION ENDED") || line.Contains("Transaction Ended")) &&
                        bufferedLines.Count > 0 && 
                        string.IsNullOrEmpty(currentTraceNumber))
                    {
                        // Set RRN to 0 for local transaction
                        currentTraceNumber = "0";
                        
                        // Use first buffered line's timestamp as transaction time
                        currentTransactionTime = bufferedLines.FirstOrDefault()?.JournalTime ?? DateTime.Now;
                        
                        // Write all buffered lines with RRN 0
                        foreach (var buffered in bufferedLines)
                        {
                            output.AppendLine($"{tid}|{currentTraceNumber}|{participantId}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(currentTransactionTime.Value)}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(buffered.JournalTime)}|" +
                                $"{buffered.Content}");
                        }
                        bufferedLines.Clear();
                        
                        // Reset for next transaction
                        currentTraceNumber = string.Empty;
                        currentTransactionTime = null;
                    }

                    // Check for transaction start - reset RRN and start buffering
                    if (line.Contains("CARD INSERTED") || 
                        line.Contains("CARD LESS SELECTED") || 
                        line.Contains("RESTART TRANSACTION"))
                    {
                        // Before clearing, check if previous transaction had buffered data but no RRN
                        if (bufferedLines.Count > 0 && string.IsNullOrEmpty(currentTraceNumber))
                        {
                            // Set RRN to 0 for previous incomplete transaction
                            currentTraceNumber = "0";
                            currentTransactionTime = bufferedLines.FirstOrDefault()?.JournalTime ?? DateTime.Now;
                            
                            // Write buffered lines with RRN 0
                            foreach (var buffered in bufferedLines)
                            {
                                output.AppendLine($"{tid}|{currentTraceNumber}|{participantId}|" +
                                    $"{DateTimeHelper.FormatTimestampForOutput(currentTransactionTime.Value)}|" +
                                    $"{DateTimeHelper.FormatTimestampForOutput(buffered.JournalTime)}|" +
                                    $"{buffered.Content}");
                            }
                        }
                        
                        // Reset RRN and transaction time for new transaction
                        currentTraceNumber = string.Empty;
                        currentTransactionTime = null;
                        inTransaction = true;
                        bufferedLines.Clear();
                    }

                    string rrn = ExtractRRN(line);
                    if (!string.IsNullOrEmpty(rrn))
                    {
                        currentTraceNumber = rrn;
                        currentTransactionTime = ExtractTransactionTime(line);

                        // Write all buffered lines with this RRN and transaction time
                        foreach (var buffered in bufferedLines)
                        {
                            output.AppendLine($"{tid}|{currentTraceNumber}|{participantId}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(currentTransactionTime.Value)}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(buffered.JournalTime)}|" +
                                $"{buffered.Content}");
                        }
                        bufferedLines.Clear();
                    }

                    if (line.Contains("[INFO:10]") || line.Contains("[PRINT:10]") || line.Contains("TEmvModule::getEmvErrorData") || line.Contains("reboot"))
                    {
                        var journalTime = ExtractJournalTime(line);
                        if (journalTime == null)
                            continue;

                        // Clean the line
                        string cleanedContent = CleanupText(line);

                        // If we have RRN, write immediately; otherwise buffer
                        if (!string.IsNullOrEmpty(currentTraceNumber) && currentTransactionTime != null)
                        {
                            output.AppendLine($"{tid}|{currentTraceNumber}|{participantId}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(currentTransactionTime.Value)}|" +
                                $"{DateTimeHelper.FormatTimestampForOutput(journalTime.Value)}|" +
                                $"{cleanedContent}");
                        }
                        else if (inTransaction)
                        {
                            // Buffer until we get RRN
                            bufferedLines.Add(new BufferedLine(journalTime.Value, cleanedContent));
                        }
                    }
                }

                // Write output file
                await File.WriteAllTextAsync(outputPath, output.ToString());
                return true;
            }
            catch (Exception ex)
            {
                lastError = $"Error processing Standard format: {ex.Message}";
                return false;
            }
        }

        /// <summary>
        /// Process file with Lengkap format (comprehensive TXT output)
        /// </summary>
        private async Task<bool> ProcessLengkapFormatAsync(string inputPath, string outputPath)
        {
            try
            {
                var lines = await File.ReadAllLinesAsync(inputPath);
                var output = new StringBuilder();
                var existingLines = new HashSet<string>();
                var cdmProcessor = new CDMProcessor();

                bool skipSupervisorSession = false;

                foreach (var line in lines)
                {
                    // Skip supervisor sessions
                    if (line.Contains("SUPERVISOR SAFE OPEN"))
                    {
                        skipSupervisorSession = true;
                        continue;
                    }

                    if (skipSupervisorSession)
                    {
                        if (line.Contains("CARD INSERTED") || line.Contains("CARD LESS SELECTED"))
                        {
                            skipSupervisorSession = false;
                        }
                        else
                        {
                            continue;
                        }
                    }

                    //Skip specific lines
                    if (ShouldSkipLine(line))
                        continue;

                    // Try to process as Cassette XML first
                    if (CassetteDetector.IsCassetteData(line))
                    {
                        string cassetteReport = CassetteDetector.ProcessCassetteXML(line);
                        if (!string.IsNullOrEmpty(cassetteReport))
                        {
                            output.AppendLine();
                            output.AppendLine(cassetteReport);
                            output.AppendLine();
                        }
                        continue;
                    }

                    // Try to process as CDM transaction
                    if (cdmProcessor.IsCDMData(line))
                    {
                        string cdmOutput = cdmProcessor.ProcessLine(line);
                        if (!string.IsNullOrEmpty(cdmOutput) && !existingLines.Contains(cdmOutput))
                        {
                            if (IsTransactionStart(cdmOutput))
                            {
                                string separator = GetTransactionSeparator(line);
                                output.Append(separator);
                            }
                            
                            output.AppendLine(cdmOutput);
                            existingLines.Add(cdmOutput);
                        }
                    }
                    // Otherwise check if line should be included
                    else if (ShouldIncludeLine(line))
                    {
                        // Filter and format line
                        string filteredLine = FilterLine(line);

                        // Avoid duplicates
                        if (!existingLines.Contains(filteredLine))
                        {
                            // Add transaction separator if needed
                            if (IsTransactionStart(filteredLine))
                            {
                                string separator = GetTransactionSeparator(line);
                                output.Append(separator);
                            }

                            output.AppendLine(filteredLine);
                            existingLines.Add(filteredLine);
                        }
                    }
                }

                // Write output file
                await File.WriteAllTextAsync(outputPath, output.ToString());
                return true;
            }
            catch (Exception ex)
            {
                lastError = $"Error processing Lengkap format: {ex.Message}";
                return false;
            }
        }

        /// <summary>
        /// Extract RRN from line containing "RRN" keyword
        /// </summary>
        public string ExtractRRN(string line)
        {
            var match = RegexPatterns.RRN.Match(line);
            if (match.Success && match.Groups.Count > 1)
            {
                return match.Groups[1].Value;
            }
            return string.Empty;
        }

        /// <summary>
        /// Extract transaction time from line (timestamp from line containing RRN)
        /// </summary>
        public DateTime? ExtractTransactionTime(string line)
        {
            var match = RegexPatterns.Timestamp.Match(line);
            if (match.Success && match.Groups.Count > 1)
            {
                return DateTimeHelper.ParseLogTimestamp(match.Groups[1].Value);
            }
            return null;
        }

        /// <summary>
        /// Extract journal time from log line
        /// </summary>
        public DateTime? ExtractJournalTime(string line)
        {
            return ExtractTransactionTime(line); // Same implementation
        }

        /// <summary>
        /// Cleanup text by removing timestamp, tags, and applying patterns
        /// </summary>
        public string CleanupText(string line)
        {
            string result = line;

            // Apply removal patterns (lines 104-109)
            foreach (var pattern in RegexPatterns.RemovalPatterns)
            {
                try
                {
                    result = pattern.Replace(result, "");
                }
                catch { }
            }

            // Extract timestamp and remove it along with tags (lines 112-122)
            var timestampMatch = RegexPatterns.TimestampWithMs.Match(line);
            if (timestampMatch.Success)
            {
                string timestamp = timestampMatch.Value;
                string remainingContent = result.Substring(timestamp.Length);

                // Remove TID tag and level tags
                remainingContent = RegexPatterns.TidTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.InfoTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.PrintTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.DebugTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.TraceTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.WarnTag.Replace(remainingContent, "");

                // Trim whitespace
                remainingContent = remainingContent.Trim();
                return remainingContent;
            }

            return result.Trim();
        }

        /// <summary>
        /// Check if line should be skipped
        /// </summary>
        private bool ShouldSkipLine(string line)
        {
            foreach (var pattern in RegexPatterns.SkipPatterns)
            {
                if (line.Contains(pattern))
                    return true;
            }

            if (line.Contains("[DEBUG:20]"))
                return true;

            return false;
        }

        /// <summary>
        /// Check if line should be included based on keyword patterns
        /// </summary>
        private bool ShouldIncludeLine(string line)
        {
            // First check skip conditions
            if (ShouldSkipLine(line))
                return false;

            // Check against keyword patterns
            foreach (var keyword in RegexPatterns.KeywordPatterns)
            {
                if (line.Contains(keyword.Replace(@"\[", "[").Replace(@"\]", "]")))
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Filter line for Lengkap format
        /// </summary>
        private string FilterLine(string line)
        {
            return CleanTimestamp(line);
        }

        /// <summary>
        /// Clean timestamp for Lengkap format
        /// </summary>
        private string CleanTimestamp(string line)
        {
            string filteredLine = line;

            // Apply remove patterns
            foreach (var pattern in RegexPatterns.RemovalPatterns)
            {
                try
                {
                    filteredLine = pattern.Replace(filteredLine, "");
                }
                catch { }
            }

            // Extract and format with timestamp
            var match = RegexPatterns.TimestampWithMs.Match(line);
            if (match.Success)
            {
                string timestamp = match.Value;
                string remainingContent = filteredLine.Substring(timestamp.Length);

                // Remove TID and level tags
                remainingContent = RegexPatterns.TidTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.InfoTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.PrintTag.Replace(remainingContent, "");
                remainingContent = RegexPatterns.DebugTag.Replace(remainingContent, "");
                remainingContent = remainingContent.Trim();

                filteredLine = timestamp + "    " + remainingContent;
            }

            return filteredLine;
        }

        /// <summary>
        /// Check if line is a transaction start
        /// </summary>
        private bool IsTransactionStart(string line)
        {
            foreach (var trigger in RegexPatterns.AllTransactionTriggers)
            {
                if (line.Contains(trigger))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Get transaction separator for Lengkap format
        /// </summary>
        private string GetTransactionSeparator(string line)
        {
            var separator = new StringBuilder();
            separator.AppendLine();
            separator.AppendLine();
            separator.AppendLine("================================================================================");

            string transactionType = "UNKNOWN TRANSACTION";
            foreach (var trigger in RegexPatterns.TransactionTriggers)
            {
                if (line.Contains(trigger))
                {
                    transactionType = trigger;
                    break;
                }
            }

            separator.AppendLine($"                           {transactionType} TRANSACTION");
            separator.AppendLine("================================================================================");

            return separator.ToString();
        }
    }
}
