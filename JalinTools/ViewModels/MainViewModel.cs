using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Input;
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

        public MainViewModel()
        {
            _parserService = new EJParserService();
            _fileService = new FileService();

            // Initialize commands
            BrowseFileCommand = new RelayCommand(_ => BrowseFile());
            ProcessCommand = new RelayCommand(_ => ProcessFile(), _ => CanProcess());
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
