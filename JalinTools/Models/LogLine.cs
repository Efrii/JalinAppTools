using System;

namespace JalinTools.Models
{
    /// <summary>
    /// Represents a single log line from the EFTS log file
    /// </summary>
    public class LogLine
    {
        public DateTime? Timestamp { get; set; }
        public string Level { get; set; } = string.Empty;
        public string Content { get; set; } = string.Empty;
        public string RRN { get; set; } = string.Empty;
        public string OriginalLine { get; set; } = string.Empty;

        public LogLine(string originalLine)
        {
            OriginalLine = originalLine;
        }

        public bool HasInfoOrPrintTag()
        {
            return OriginalLine.Contains("[INFO:10]") || OriginalLine.Contains("[PRINT:10]");
        }

        public bool HasDebugTag()
        {
            return OriginalLine.Contains("[DEBUG:20]");
        }
    }
}
