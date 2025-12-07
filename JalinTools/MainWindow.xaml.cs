using System.Windows;
using MaterialDesignThemes.Wpf;

namespace JalinTools
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            
            // Navigate to home page on startup
            if (DataContext is ViewModels.NavigationViewModel navViewModel)
            {
                navViewModel.NavigateToHome();
            }

            // Update maximize icon on state change
            StateChanged += (s, e) => UpdateMaximizeIcon();
        }

        private void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        private void MaximizeButton_Click(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState == WindowState.Maximized 
                ? WindowState.Normal 
                : WindowState.Maximized;
            UpdateMaximizeIcon();
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void UpdateMaximizeIcon()
        {
            if (MaximizeIcon != null)
            {
                MaximizeIcon.Kind = WindowState == WindowState.Maximized 
                    ? PackIconKind.WindowRestore 
                    : PackIconKind.WindowMaximize;
                
                MaximizeButton.ToolTip = WindowState == WindowState.Maximized 
                    ? "Restore" 
                    : "Maximize";
            }
        }
    }
}
