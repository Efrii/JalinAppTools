using System.Windows;
using System.Windows.Controls;

namespace JalinTools.Views
{
    /// <summary>
    /// Interaction logic for HomePage.xaml
    /// </summary>
    public partial class HomePage : Page
    {
        public HomePage()
        {
            InitializeComponent();
        }

        private void OpenEJParser_Click(object sender, RoutedEventArgs e)
        {
            // Navigate to EJ Parser page
            var mainWindow = Window.GetWindow(this) as MainWindow;
            if (mainWindow?.DataContext is ViewModels.NavigationViewModel navViewModel)
            {
                navViewModel.NavigateToEJParser();
            }
        }
    }
}
