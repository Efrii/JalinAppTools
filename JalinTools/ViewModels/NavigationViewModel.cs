using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Controls;

namespace JalinTools.ViewModels
{
    /// <summary>
    /// Navigation ViewModel for managing page navigation
    /// </summary>
    public class NavigationViewModel : INotifyPropertyChanged
    {
        private Page? _currentPage;

        public Page? CurrentPage
        {
            get => _currentPage;
            set
            {
                _currentPage = value;
                OnPropertyChanged();
            }
        }

        public void NavigateToHome()
        {
            CurrentPage = new Views.HomePage();
        }

        public void NavigateToEJParser()
        {
            CurrentPage = new Views.EJParserPage();
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
