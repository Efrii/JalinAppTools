using System;
using System.Collections.Generic;

namespace JalinTools.Models
{
    /// <summary>
    /// Represents a transaction block with buffered lines for Standard format
    /// Used for transaction buffering logic
    /// </summary>
    public class TransactionBlock
    {
        public string TraceNumber { get; set; } = string.Empty;
        public DateTime? TransactionTime { get; set; }
        public List<BufferedLine> BufferedLines { get; set; } = new List<BufferedLine>();
    }

    /// <summary>
    /// Represents a buffered line with journal time and content
    /// </summary>
    public class BufferedLine
    {
        public DateTime JournalTime { get; set; }
        public string Content { get; set; } = string.Empty;

        public BufferedLine(DateTime journalTime, string content)
        {
            JournalTime = journalTime;
            Content = content;
        }
    }
}
