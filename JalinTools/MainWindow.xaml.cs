using System.Windows;

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
        }
    }
}
