using System;
using System.Globalization;

namespace JalinTools.Helpers
{
    /// <summary>
    /// Helper methods for date and time formatting
    /// </summary>
    public static class DateTimeHelper
    {
        /// <summary>
        /// Format date for output filename: DDMMYYYY
        /// </summary>
        public static string FormatDateForFilename(DateTime date)
        {
            return date.ToString("ddMMyyyy");
        }

        /// <summary>
        /// Format timestamp for output: DD.MM.YYYY HH:MM:SS (without milliseconds)
        /// </summary>
        public static string FormatTimestampForOutput(DateTime dateTime)
        {
            return dateTime.ToString("dd.MM.yyyy HH:mm:ss");
        }

        /// <summary>
        /// Parse timestamp from log line: DD.MM.YYYY HH:MM:SS.mmm
        /// </summary>
        public static DateTime? ParseLogTimestamp(string timestampStr)
        {
            if (string.IsNullOrEmpty(timestampStr))
                return null;

            // Try with milliseconds first
            if (DateTime.TryParseExact(timestampStr, "dd.MM.yyyy HH:mm:ss.fff",
                CultureInfo.InvariantCulture, DateTimeStyles.None, out DateTime result))
            {
                return result;
            }

            // Try without milliseconds
            if (DateTime.TryParseExact(timestampStr, "dd.MM.yyyy HH:mm:ss",
                CultureInfo.InvariantCulture, DateTimeStyles.None, out result))
            {
                return result;
            }

            return null;
        }
    }
}
