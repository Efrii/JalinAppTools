using System.Windows;
using System.Windows.Controls;

namespace JalinTools.Views
{
    /// <summary>
    /// Interaction logic for EJParserPage.xaml
    /// </summary>
    public partial class EJParserPage : Page
    {
        public EJParserPage()
        {
            InitializeComponent();
        }

        private void BackToHome_Click(object sender, RoutedEventArgs e)
        {
            // Navigate back to home
            var mainWindow = Window.GetWindow(this) as MainWindow;
            if (mainWindow?.DataContext is ViewModels.NavigationViewModel navViewModel)
            {
                navViewModel.NavigateToHome();
            }
        }
    }
}
