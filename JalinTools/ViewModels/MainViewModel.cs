using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Linq;
using JalinTools.Models;
using JalinTools.Services;
using Microsoft.Win32;

namespace JalinTools.ViewModels
{
    /// <summary>
    /// Main ViewModel for EJ Parser application
    /// Implements INotifyPropertyChanged for data binding
    /// </summary>
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly EJParserService _parserService;
        private readonly FileService _fileService;

        // Backing fields
        private string _inputFilePath = string.Empty;
        private string _tid = string.Empty;
        private DateTime _selectedDate = DateTime.Now;
        private string _selectedParticipantId = "200";
        private EJParserFormat _selectedFormat = EJParserFormat.Lengkap;
        private string _statusMessage = "Siap";
        private bool _isProcessing = false;
        private string _outputPreview = string.Empty;
        private string _previewContent = string.Empty;
        private bool _isPreviewVisible = false;
        private bool _isPreviewExpanded = false;
        private string _currentOutputPath = string.Empty;

        public MainViewModel()
        {
            _parserService = new EJParserService();
            _fileService = new FileService();

            // Initialize commands
            BrowseFileCommand = new RelayCommand(_ => BrowseFile());
            ProcessCommand = new RelayCommand(_ => ProcessFile(), _ => CanProcess());
            OpenFileCommand = new RelayCommand(_ => OpenOutputFile());
            TogglePreviewCommand = new RelayCommand(_ => TogglePreview());
            ToggleExpandCommand = new RelayCommand(_ => ToggleExpand());
        }

        #region Properties

        public string InputFilePath
        {
            get => _inputFilePath;
            set
            {
                _inputFilePath = value;
                OnPropertyChanged();
                CommandManager.InvalidateRequerySuggested();
            }
        }

        public string TID
        {
            get => _tid;
            set
            {
                _tid = value;
                OnPropertyChanged();
                CommandManager.InvalidateRequerySuggested();
            }
        }

        public DateTime SelectedDate
        {
            get => _selectedDate;
            set
            {
                _selectedDate = value;
                OnPropertyChanged();
            }
        }

        public string SelectedParticipantId
        {
            get => _selectedParticipantId;
            set
            {
                _selectedParticipantId = value;
                OnPropertyChanged();
            }
        }

        public EJParserFormat SelectedFormat
        {
            get => _selectedFormat;
            set
            {
                _selectedFormat = value;
                OnPropertyChanged();
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                _statusMessage = value;
                OnPropertyChanged();
            }
        }

        public bool IsProcessing
        {
            get => _isProcessing;
            set
            {
                _isProcessing = value;
                OnPropertyChanged();
                CommandManager.InvalidateRequerySuggested();
            }
        }

        public string OutputPreview
        {
            get => _outputPreview;
            set
            {
                _outputPreview = value;
                OnPropertyChanged();
            }
        }

        public string PreviewContent
        {
            get => _previewContent;
            set
            {
                _previewContent = value;
                OnPropertyChanged();
            }
        }

        public bool IsPreviewVisible
        {
            get => _isPreviewVisible;
            set
            {
                _isPreviewVisible = value;
                OnPropertyChanged();
            }
        }

        public string CurrentOutputPath
        {
            get => _currentOutputPath;
            set
            {
                _currentOutputPath = value;
                OnPropertyChanged();
            }
        }

        public bool IsPreviewExpanded
        {
            get => _isPreviewExpanded;
            set
            {
                _isPreviewExpanded = value;
                OnPropertyChanged();
            }
        }

        // For RadioButton binding
        public bool IsStandardFormat
        {
            get => SelectedFormat == EJParserFormat.Standard;
            set
            {
                if (value)
                    SelectedFormat = EJParserFormat.Standard;
            }
        }

        public bool IsLengkapFormat
        {
            get => SelectedFormat == EJParserFormat.Lengkap;
            set
            {
                if (value)
                    SelectedFormat = EJParserFormat.Lengkap;
            }
        }

        #endregion

        #region Commands

        public ICommand BrowseFileCommand { get; }
        public ICommand ProcessCommand { get; }
        public ICommand OpenFileCommand { get; }
        public ICommand TogglePreviewCommand { get; }
        public ICommand ToggleExpandCommand { get; }

        #endregion

        #region Command Implementations

        /// <summary>
        /// Browse for input file
        /// </summary>
        private void BrowseFile()
        {
            var openFileDialog = new OpenFileDialog
            {
                Title = "Pilih File Input EJ",
                Filter = "File Teks (*.txt)|*.txt|Semua File (*.*)|*.*",
                FilterIndex = 1
            };

            if (openFileDialog.ShowDialog() == true)
            {
                InputFilePath = openFileDialog.FileName;
                StatusMessage = "File input dipilih";
            }
        }

        /// <summary>
        /// Check if processing can be executed
        /// </summary>
        private bool CanProcess()
        {
            return !string.IsNullOrWhiteSpace(InputFilePath) &&
                   !string.IsNullOrWhiteSpace(TID) &&
                   !IsProcessing;
        }

        /// <summary>
        /// Process EJ file
        /// </summary>
        private async void ProcessFile()
        {
            try
            {
                IsProcessing = true;
                StatusMessage = $"Memproses: {TID} - {SelectedDate:ddMMyyyy}";
                OutputPreview = string.Empty;

                // Determine file extension based on format - both use txt
                string extension = "txt";

                // Get output file path
                string outputPath = _fileService.GetOutputFilePath(TID, SelectedDate, extension);

                // Process file
                bool success = await _parserService.ProcessFileAsync(
                    InputFilePath,
                    outputPath,
                    TID,
                    SelectedParticipantId,
                    SelectedFormat);

                if (success)
                {
                    StatusMessage = "Pemrosesan selesai - File berhasil disimpan";
                    OutputPreview = $"Disimpan di: {outputPath}";

                    // Load preview automatically
                    await LoadPreviewAsync(outputPath);

                    MessageBox.Show(
                        $"File EJ berhasil diproses!\n\n" +
                        $"File output: {outputPath}\n\n" +
                        $"TID: {TID}\n" +
                        $"Tanggal: {SelectedDate:ddMMyyyy}",
                        "Berhasil",
                        MessageBoxButton.OK,
                        MessageBoxImage.Information);
                }
                else
                {
                    StatusMessage = "Error: Pemrosesan gagal";
                    string error = _parserService.GetLastError();

                    MessageBox.Show(
                        $"Gagal memproses file EJ.\n\n" +
                        $"Error: {error}",
                        "Error",
                        MessageBoxButton.OK,
                        MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                StatusMessage = "Error: Pemrosesan gagal";
                MessageBox.Show(
                    $"Terjadi kesalahan:\n\n{ex.Message}",
                    "Error",
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }
            finally
            {
                IsProcessing = false;
            }
        }

        /// <summary>
        /// Open output file in default editor
        /// </summary>
        private void OpenOutputFile()
        {
            if (!string.IsNullOrEmpty(CurrentOutputPath) && System.IO.File.Exists(CurrentOutputPath))
            {
                try
                {
                    System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = CurrentOutputPath,
                        UseShellExecute = true
                    });
                }
                catch (Exception ex)
                {
                    MessageBox.Show(
                        $"Gagal membuka file:\n\n{ex.Message}",
                        "Error",
                        MessageBoxButton.OK,
                        MessageBoxImage.Error);
                }
            }
        }

        /// <summary>
        /// Toggle preview panel visibility
        /// </summary>
        private void TogglePreview()
        {
            IsPreviewVisible = !IsPreviewVisible;
        }

        /// <summary>
        /// Toggle preview panel expansion
        /// </summary>
        private void ToggleExpand()
        {
            IsPreviewExpanded = !IsPreviewExpanded;
        }

        /// <summary>
        /// Load file content for preview
        /// </summary>
        private async Task LoadPreviewAsync(string filePath)
        {
            try
            {
                // Read first 1000 lines for preview
                var lines = await _fileService.ReadLinesAsync(filePath);
                var previewLines = lines.Take(1000).ToArray();
                
                PreviewContent = string.Join(Environment.NewLine, previewLines);
                
                if (lines.Count > 1000)
                {
                    PreviewContent += $"\n\n... ({lines.Count - 1000} baris lainnya tidak ditampilkan)";
                }

                CurrentOutputPath = filePath;
                IsPreviewVisible = true;
            }
            catch (Exception ex)
            {
                PreviewContent = $"Error loading preview: {ex.Message}";
            }
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler? PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion
    }
}
