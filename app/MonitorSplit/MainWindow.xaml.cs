// MainWindow.xaml.cs — Code-behind for the main window
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using MonitorSplit.ViewModels;
using MonitorSplit.Converters;

namespace MonitorSplit;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();

        // Register value converters as static resources
        Resources["InverseBoolConverter"] = new InverseBoolConverter();
        Resources["NullToCollapsedConverter"] = new NullToCollapsedConverter();
        Resources["RatioToGridLengthConverter"] = new RatioToGridLengthConverter();
    }

    // ─── Custom title bar drag ────────────────────────────────────────
    private void TitleBar_MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left)
            DragMove();
    }

    private void MinimizeBtn_Click(object sender, RoutedEventArgs e)
    {
        // Hide window (stays in tray)
        Hide();
    }

    private void CloseBtn_Click(object sender, RoutedEventArgs e)
    {
        // Close to tray, not exit — use Application.Current.Shutdown() to fully exit
        Hide();
    }
}
