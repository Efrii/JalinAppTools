using System.Text.RegularExpressions;

namespace JalinTools.Helpers
{
    /// <summary>
    /// Regex patterns for parsing EJ log files
    /// </summary>
    public static class RegexPatterns
    {
        // Extraction patterns
        public static readonly Regex RRN = new Regex(@"RRN\s+(\d+)", RegexOptions.Compiled);
        public static readonly Regex Timestamp = new Regex(@"(\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2})", RegexOptions.Compiled);
        public static readonly Regex TimestampWithMs = new Regex(@"^(\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3})", RegexOptions.Compiled);
        
        // Tag patterns for removal
        public static readonly Regex TidTag = new Regex(@"\[TID:0x[0-9A-Fa-f]+\]", RegexOptions.Compiled);
        public static readonly Regex InfoTag = new Regex(@"\[INFO:\d+\]", RegexOptions.Compiled);
        public static readonly Regex PrintTag = new Regex(@"\[PRINT:\d+\]", RegexOptions.Compiled);
        public static readonly Regex DebugTag = new Regex(@"\[DEBUG:\d+\]", RegexOptions.Compiled);
        public static readonly Regex TraceTag = new Regex(@"\[TRACE:\d+\]", RegexOptions.Compiled);
        
        // Combined removal patterns
        public static readonly Regex[] RemovalPatterns = new Regex[]
        {
            new Regex(@"\[TID:0x[A-Fa-f0-9]+\] \[INFO:\d+\]", RegexOptions.Compiled),
            new Regex(@"\[TID:0x[A-Fa-f0-9]+\] \[PRINT:\d+\]", RegexOptions.Compiled),
            new Regex(@"\[INFO:\d+\] Motorized card reader detected", RegexOptions.Compiled),
            new Regex(@"\[DEBUG:20\]", RegexOptions.Compiled),
            new Regex(@"XFS WFSAsyncExecute.*CMD: 30[23].*ReqID", RegexOptions.Compiled),
            new Regex(@"\(class\w+\.cpp:\d+\)", RegexOptions.Compiled)
        };

        // Keyword patterns for filtering
        public static readonly string[] KeywordPatterns = new string[]
        {
            "CARD INSERTED",
            @"\[INFO:10\]",
            @"\[PRINT:10\]",
            @"\[XML:15\]",
            "WFS_CMD_CDM_DISPENSE",
            "WFS_CMD_CDM_PRESENT",
            "EventID: 302",
            "EventID: 303",
            "DSP--"
        };

        // Skip patterns
        public static readonly string[] SkipPatterns = new string[]
        {
            "Motorized card reader detected",
            "WFS_GETINFO_COMPLETE"
        };

        // Transaction triggers
        public static readonly string[] TransactionTriggers = new string[]
        {
            "CARD INSERTED",
            "CARD LESS SELECTED"
        };

        public static readonly string[] AllTransactionTriggers = new string[]
        {
            "CARD INSERTED",
            "SUPERVISOR SAFE OPEN",
            "CARD LESS SELECTED"
        };
    }
}
