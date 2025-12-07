using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace JalinTools.Models
{
    /// <summary>
    /// CDM Transaction data structure
    /// </summary>
    public class CDMTransaction
    {
        public CDMRequest Request { get; set; } = new CDMRequest();
        public CDMResponse Response { get; set; } = new CDMResponse();
        public bool IsComplete { get; set; } = false;
    }

    public class CDMRequest
    {
        public string ReqId { get; set; } = string.Empty;
        public string Timestamp { get; set; } = string.Empty;
        public string Command { get; set; } = string.Empty;
        public string ServiceProvider { get; set; } = string.Empty;
        public bool HasError { get; set; } = false;
        public string ErrorMessage { get; set; } = string.Empty;
        public string MStatus { get; set; } = string.Empty;
    }

    public class CDMResponse
    {
        public string ReqId { get; set; } = string.Empty;
        public string Timestamp { get; set; } = string.Empty;
        public string Result { get; set; } = string.Empty;
    }
}
