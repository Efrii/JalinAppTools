using System;
using System.Diagnostics;
using System.IO;
using System.Windows;

namespace JalinTools.Views
{
    /// <summary>
    /// Interaction logic for PreviewWindow.xaml
    /// </summary>
    public partial class PreviewWindow : Window
    {
        private readonly string? filePath;

        public PreviewWindow(string content, string? outputPath = null)
        {
            InitializeComponent();

            ContentTextBox.Text = content;
            filePath = outputPath;

            // Update file path display
            if (!string.IsNullOrEmpty(filePath))
            {
                FilePathText.Text = Path.GetFileName(filePath);
            }

            // Update line count
            var lineCount = content.Split('\n').Length;
            LineCountText.Text = $"Total {lineCount:N0} baris";
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void CopyButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Clipboard.SetText(ContentTextBox.Text);
                MessageBox.Show("Konten berhasil disalin ke clipboard!", 
                    "Sukses", 
                    MessageBoxButton.OK, 
                    MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Gagal menyalin ke clipboard: {ex.Message}", 
                    "Error", 
                    MessageBoxButton.OK, 
                    MessageBoxImage.Error);
            }
        }

        private void OpenFileButton_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(filePath) || !File.Exists(filePath))
            {
                MessageBox.Show("File tidak ditemukan!", 
                    "Error", 
                    MessageBoxButton.OK, 
                    MessageBoxImage.Warning);
                return;
            }

            try
            {
                // Open with notepad explicitly for TXT files
                Process.Start(new ProcessStartInfo
                {
                    FileName = "notepad.exe",
                    Arguments = $"\"{filePath}\"",
                    UseShellExecute = false
                });
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Gagal membuka file: {ex.Message}", 
                    "Error", 
                    MessageBoxButton.OK, 
                    MessageBoxImage.Error);
            }
        }
    }
}
