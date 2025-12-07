using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace JalinTools.Services
{
    /// <summary>
    /// Detects and formats cassette information from XML data
    /// Based on C++ cassette_detector implementation
    /// </summary>
    public class CassetteDetector
    {
        private const int MAX_CASSETTES = 5;
        
        private readonly string xmlData;
        private readonly int[] dispensedValues = new int[MAX_CASSETTES];
        private readonly int[] rejectedValues = new int[MAX_CASSETTES];
        private readonly int[] remainingValues = new int[MAX_CASSETTES];
        private readonly int[] denominations = new int[MAX_CASSETTES];
        private readonly int[] initialValues = new int[MAX_CASSETTES];
        private readonly string[] denominationsView = new string[MAX_CASSETTES];
        private readonly int[] cassetteTypes = new int[MAX_CASSETTES];
        private readonly Dictionary<int, string> cashTotalLabels = new Dictionary<int, string>();
        private readonly Dictionary<int, int> denominationValues = new Dictionary<int, int>();

        public CassetteDetector(string data)
        {
            xmlData = data;
            for (int i = 0; i < MAX_CASSETTES; i++)
            {
                denominationsView[i] = "";
            }
        }

        public string DetectCassette()
        {
            ParseXMLData();
            
            var sb = new StringBuilder();
            
            sb.AppendLine("================================================================================");
            sb.AppendLine("                     LAPORAN STATUS CASH DISPENSER");
            sb.AppendLine("================================================================================");
            
            // Find active cassettes
            var activeCassettes = new List<int>();
            for (int i = 0; i < MAX_CASSETTES; i++)
            {
                if (initialValues[i] > 0 || dispensedValues[i] > 0 || rejectedValues[i] > 0 || remainingValues[i] > 0)
                {
                    activeCassettes.Add(i);
                }
            }
            
            // Find financial cassettes (not reject/retract)
            var financialCassettes = activeCassettes
                .Where(i => cassetteTypes[i] != 2 && cassetteTypes[i] != 6)
                .ToList();
            
            if (activeCassettes.Count == 0)
            {
                sb.AppendLine("Tidak ada kaset aktif terdeteksi.");
                sb.AppendLine("================================================================================");
                return sb.ToString();
            }
            
            // CASSETTE SUMMARY TABLE
            sb.AppendLine();
            sb.AppendLine("RINGKASAN KASET:");
            sb.AppendLine("--------------------------------------------------------------------------------");
            sb.AppendLine("KASET           DENOMINASI      AWAL       KELUAR      REJECT      SISA");
            sb.AppendLine("--------------------------------------------------------------------------------");
            
            foreach (var cassetteIndex in activeCassettes)
            {
                var label = cashTotalLabels.ContainsKey(cassetteIndex + 1)
                    ? cashTotalLabels[cassetteIndex + 1]
                    : $"CASSETTE_{cassetteIndex + 1}";
                
                sb.AppendLine(
                    $"{label.PadRight(16)}" +
                    $"{denominationsView[cassetteIndex].PadRight(16)}" +
                    $"{initialValues[cassetteIndex].ToString().PadRight(11)}" +
                    $"{dispensedValues[cassetteIndex].ToString().PadRight(12)}" +
                    $"{rejectedValues[cassetteIndex].ToString().PadRight(12)}" +
                    $"{remainingValues[cassetteIndex]}");
            }
            
            sb.AppendLine("--------------------------------------------------------------------------------");
            
            // SUMMARY PER DISPENSER CASSETTE
            int totalInitial = 0;
            int totalDispensed = 0;
            int totalRejected = 0;
            int totalRemaining = 0;
            bool hasDiscrepancy = false;
            
            sb.AppendLine();
            sb.AppendLine("RINGKASAN PER KASET DISPENSER:");
            sb.AppendLine("--------------------------------------------------------------------------------");
            
            if (financialCassettes.Count == 0)
            {
                sb.AppendLine("Tidak ada kaset dispenser (hanya kaset reject/retract terdeteksi).");
            }
            else
            {
                foreach (var cassetteIndex in financialCassettes)
                {
                    var label = cashTotalLabels.ContainsKey(cassetteIndex + 1)
                        ? cashTotalLabels[cassetteIndex + 1]
                        : $"CASSETTE_{cassetteIndex + 1}";
                    
                    int denomination = denominations[cassetteIndex];
                    int initial = initialValues[cassetteIndex] * denomination;
                    int dispensed = dispensedValues[cassetteIndex] * denomination;
                    int rejected = rejectedValues[cassetteIndex] * denomination;
                    int remaining = remainingValues[cassetteIndex] * denomination;
                    int total = remaining + dispensed + rejected;
                    
                    string typeInfo = cassetteTypes[cassetteIndex] == 3 ? " (DISPENSER)" : "";
                    
                    sb.AppendLine($"{label}{typeInfo}:");
                    sb.AppendLine($"  Jumlah Awal     : {FormatCurrency(initial, "Rp")}");
                    sb.AppendLine($"  Jumlah Keluar   : {FormatCurrency(dispensed, "Rp")}");
                    sb.AppendLine($"  Jumlah Reject   : {FormatCurrency(rejected, "Rp")}");
                    sb.AppendLine($"  Jumlah Sisa     : {FormatCurrency(remaining, "Rp")}");
                    sb.AppendLine($"  Total Terhitung : {FormatCurrency(total, "Rp")}");
                    
                    if (initial > 0 && total != initial)
                    {
                        hasDiscrepancy = true;
                        sb.AppendLine();
                        sb.AppendLine("  *** DICURIGAI ADA JAMMED ***");
                        sb.AppendLine($"  TOTAL COUNTER TIDAK COCOK {FormatCurrency(total, "Rp")}" +
                            $" DENGAN JUMLAH AWAL {FormatCurrency(initial, "Rp")}" +
                            $" PADA {label}");
                        sb.AppendLine($"  SELISIH: {FormatCurrency(Math.Abs(total - initial), "Rp")}");
                    }
                    
                    totalInitial += initial;
                    totalDispensed += dispensed;
                    totalRejected += rejected;
                    totalRemaining += remaining;
                }
            }
            
            // SUMMARY BY DENOMINATION
            if (denominationValues.Count > 1 && financialCassettes.Count > 0)
            {
                sb.AppendLine();
                sb.AppendLine("RINGKASAN PER DENOMINASI:");
                sb.AppendLine("--------------------------------------------------------------------------------");
                
                foreach (var kvp in denominationValues.OrderByDescending(x => x.Value))
                {
                    int denomination = kvp.Value / 100;
                    if (denomination == 0) continue;
                    
                    int denomInitial = 0, denomDispensed = 0, denomRejected = 0, denomRemaining = 0;
                    
                    for (int i = 0; i < MAX_CASSETTES; i++)
                    {
                        if (denominations[i] == denomination && cassetteTypes[i] != 2 && cassetteTypes[i] != 6)
                        {
                            denomInitial += initialValues[i] * denomination;
                            denomDispensed += dispensedValues[i] * denomination;
                            denomRejected += rejectedValues[i] * denomination;
                            denomRemaining += remainingValues[i] * denomination;
                        }
                    }
                    
                    if (denomInitial > 0 || denomDispensed > 0 || denomRejected > 0 || denomRemaining > 0)
                    {
                        sb.AppendLine($"Denominasi {FormatCurrency(denomination, "")} :");
                        sb.AppendLine($"  Awal   : {FormatCurrency(denomInitial, "Rp")}");
                        sb.AppendLine($"  Keluar : {FormatCurrency(denomDispensed, "Rp")}");
                        sb.AppendLine($"  Reject : {FormatCurrency(denomRejected, "Rp")}");
                        sb.AppendLine($"  Sisa   : {FormatCurrency(denomRemaining, "Rp")}");
                        
                        int denomTotal = denomRemaining + denomDispensed + denomRejected;
                        if (denomInitial > 0 && denomTotal != denomInitial)
                        {
                            hasDiscrepancy = true;
                            sb.AppendLine();
                            sb.AppendLine("  *** DICURIGAI ADA JAMMED ***");
                            sb.AppendLine($"  TOTAL COUNTER TIDAK COCOK {FormatCurrency(denomTotal, "Rp")}" +
                                $" DENGAN JUMLAH AWAL {FormatCurrency(denomInitial, "Rp")}" +
                                $" PADA DENOMINASI {FormatCurrency(denomination, "")}");
                        }
                    }
                }
            }
            
            // OVERALL SUMMARY
            if (financialCassettes.Count > 0)
            {
                sb.AppendLine();
                sb.AppendLine("RINGKASAN TOTAL CASH DISPENSER:");
                sb.AppendLine("--------------------------------------------------------------------------------");
                sb.AppendLine($"Total Jumlah Awal       : {FormatCurrency(totalInitial, "Rp")}");
                sb.AppendLine($"Total Jumlah Keluar     : {FormatCurrency(totalDispensed, "Rp")}");
                sb.AppendLine($"Total Jumlah Reject     : {FormatCurrency(totalRejected, "Rp")}");
                sb.AppendLine($"Total Jumlah Sisa       : {FormatCurrency(totalRemaining, "Rp")}");
                sb.AppendLine($"Total dalam Dispenser   : {FormatCurrency(totalDispensed + totalRejected + totalRemaining, "Rp")}");
                
                int overallTotal = totalDispensed + totalRejected + totalRemaining;
                if (totalInitial > 0 && overallTotal != totalInitial)
                {
                    hasDiscrepancy = true;
                    sb.AppendLine();
                    sb.AppendLine("*** KETIDAKCOCOKAN TOTAL TERDETEKSI ***");
                    sb.AppendLine($"Total Yang Diharapkan: {FormatCurrency(totalInitial, "Rp")}");
                    sb.AppendLine($"Total Aktual:          {FormatCurrency(overallTotal, "Rp")}");
                    sb.AppendLine($"Selisih:               {FormatCurrency(Math.Abs(overallTotal - totalInitial), "Rp")}");
                }
            }
            
            if (financialCassettes.Count > 0 && hasDiscrepancy)
            {
                sb.AppendLine();
                sb.AppendLine("*** PERHATIAN: KETIDAKCOCOKAN KASET TERDETEKSI ***");
                sb.AppendLine("Silakan periksa dispenser untuk kemungkinan jamming atau masalah mekanis.");
            }
            
            sb.AppendLine();
            sb.Append("================================================================================");
            
            return sb.ToString();
        }

        private void ParseXMLData()
        {
            // Clean timestamp and tags
            var timestampPattern = new Regex(@"^\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}:\d{2}\.\d{3} \[TID:0x[0-9A-Fa-f]+\] \[XML:\d+\] ");
            string cleanedXML = timestampPattern.Replace(xmlData, "");
            
            // Add quotes to attributes
            var attrPattern = new Regex(@"(\w+)=([\w|]+)");
            cleanedXML = attrPattern.Replace(cleanedXML, "$1='$2'");
            
            ParseXMLElements(cleanedXML);
        }

        private void ParseXMLElements(string xml)
        {
            var cuPattern = new Regex(@"<cu\s+([^>]+)>");
            var matches = cuPattern.Matches(xml);
            
            int dcIndex = 1;
            
            foreach (Match match in matches)
            {
                var attributes = match.Groups[1].Value;
                var attrs = ParseAttributes(attributes);
                
                if (!attrs.ContainsKey("n")) continue;
                
                int vIndex = int.Parse(attrs["n"]) - 1;
                if (vIndex < 0 || vIndex >= MAX_CASSETTES) continue;
                
                int typIndex = int.Parse(attrs["typ"]);
                int denominationValue = int.Parse(attrs["v"]);
                string dcView = attrs.ContainsKey("dc") ? attrs["dc"] : "0";
                
                int parsedDcIndex = dcIndex;
                if (dcView != "0" && !string.IsNullOrEmpty(dcView) && char.IsDigit(dcView[0]))
                {
                    parsedDcIndex = int.Parse(dcView);
                }
                else
                {
                    parsedDcIndex = dcIndex++;
                }
                
                if (!cashTotalLabels.ContainsKey(parsedDcIndex))
                {
                    if (typIndex == 2)
                        cashTotalLabels[parsedDcIndex] = "REJECT_" + dcView;
                    else if (typIndex == 3)
                        cashTotalLabels[parsedDcIndex] = "CASSETTE_" + dcView;
                    else if (typIndex == 6)
                        cashTotalLabels[parsedDcIndex] = "RETRACT_" + dcView;
                }
                
                if (!denominationValues.ContainsKey(vIndex))
                {
                    denominationValues[vIndex] = denominationValue;
                }
                
                initialValues[vIndex] = attrs.ContainsKey("ic") ? int.Parse(attrs["ic"]) : 0;
                dispensedValues[vIndex] = attrs.ContainsKey("ds") ? int.Parse(attrs["ds"]) : 0;
                rejectedValues[vIndex] = attrs.ContainsKey("rj") ? int.Parse(attrs["rj"]) : 0;
                remainingValues[vIndex] = attrs.ContainsKey("c") ? int.Parse(attrs["c"]) : 0;
                denominations[vIndex] = denominationValue / 100;
                denominationsView[vIndex] = FormatCurrency(denominationValue / 100, "");
                cassetteTypes[vIndex] = typIndex;
            }
        }

        private Dictionary<string, string> ParseAttributes(string attrs)
        {
            var result = new Dictionary<string, string>();
            var attrRegex = new Regex(@"(\w+)=['""](.+?)['""]");
            var matches = attrRegex.Matches(attrs);
            
            foreach (Match match in matches)
            {
                result[match.Groups[1].Value] = match.Groups[2].Value;
            }
            
            return result;
        }

        private string FormatCurrency(int value, string currency)
        {
            if (value == 0) return currency + (string.IsNullOrEmpty(currency) ? "" : " ") + "0";
            
            string numStr = value.ToString();
            string formatted = "";
            int count = 0;
            
            for (int i = numStr.Length - 1; i >= 0; i--)
            {
                if (count > 0 && count % 3 == 0)
                {
                    formatted = "." + formatted;
                }
                formatted = numStr[i] + formatted;
                count++;
            }
            
            return string.IsNullOrEmpty(currency) ? formatted : currency + " " + formatted;
        }

        public static bool IsCassetteData(string line)
        {
            return line.Contains("[XML:15]") && line.Contains("<cunits");
        }

        public static string ProcessCassetteXML(string line)
        {
            if (!IsCassetteData(line)) return string.Empty;
            
            var detector = new CassetteDetector(line);
            return detector.DetectCassette();
        }
    }
}
