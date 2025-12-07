using System;
using System.IO;
using System.Threading.Tasks;

namespace JalinTools.Services
{
    /// <summary>
    /// File service for I/O operations
    /// </summary>
    public class FileService
    {
        /// <summary>
        /// Read log file asynchronously
        /// </summary>
        public async Task<string[]> ReadLogFileAsync(string filePath)
        {
            if (!File.Exists(filePath))
                throw new FileNotFoundException($"Input file not found: {filePath}");

            return await File.ReadAllLinesAsync(filePath);
        }

        /// <summary>
        /// Write output file asynchronously
        /// </summary>
        public async Task WriteOutputFileAsync(string filePath, string content)
        {
            // Create directory if it doesn't exist
            var directory = Path.GetDirectoryName(filePath);
            if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            await File.WriteAllTextAsync(filePath, content);
        }

        /// <summary>
        /// Create output directory if it doesn't exist
        /// </summary>
        public void CreateOutputDirectory(string directoryPath)
        {
            if (!Directory.Exists(directoryPath))
            {
                Directory.CreateDirectory(directoryPath);
            }
        }

        /// <summary>
        /// Get output file path based on application directory
        /// </summary>
        public string GetOutputFilePath(string tid, DateTime date, string extension)
        {
            // Get application directory
            string appDirectory = AppDomain.CurrentDomain.BaseDirectory;
            
            // Create EJParse folder path
            string outputFolder = Path.Combine(appDirectory, "EJParse");
            
            // Create folder if it doesn't exist
            CreateOutputDirectory(outputFolder);
            
            // Format date as DDMMYYYY
            string formattedDate = date.ToString("ddMMyyyy");
            
            // Generate filename: EJ_TID_DDMMYYYY.ext
            string filename = $"EJ_{tid}_{formattedDate}.{extension}";
            
            return Path.Combine(outputFolder, filename);
        }
    }
}
