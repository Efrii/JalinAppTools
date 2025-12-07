using System.Windows;
using System.Windows.Input;
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

        private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                // Double click to maximize/restore
                MaximizeButton_Click(sender, e);
            }
            else
            {
                // Single click to drag
                DragMove();
            }
        }

        private void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        private void MaximizeButton_Click(object sender, RoutedEventArgs e)
        {
            if (WindowState == WindowState.Maximized)
            {
                WindowState = WindowState.Normal;
            }
            else
            {
                WindowState = WindowState.Maximized;
            }
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
