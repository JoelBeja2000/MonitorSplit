// App.xaml.cs — Application entry point and lifetime management
using System.Windows;
using Microsoft.Extensions.DependencyInjection;
using MonitorSplit.Services;
using MonitorSplit.ViewModels;

namespace MonitorSplit;

public partial class App : Application
{
    private ServiceProvider? _services;
    private System.Windows.Forms.NotifyIcon? _trayIcon;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // ─── Dependency Injection setup ──────────────────────────────
        var services = new ServiceCollection();
        services.AddSingleton<DriverManager>();
        services.AddSingleton<MainViewModel>();
        services.AddSingleton<MainWindow>();
        _services = services.BuildServiceProvider();

        // ─── Setup system tray icon ──────────────────────────────────
        SetupTrayIcon();

        // ─── Show main window ────────────────────────────────────────
        var mainWindow = _services.GetRequiredService<MainWindow>();
        mainWindow.DataContext = _services.GetRequiredService<MainViewModel>();
        mainWindow.Show();
    }

    private void SetupTrayIcon()
    {
        _trayIcon = new System.Windows.Forms.NotifyIcon
        {
            Text    = "MonitorSplit",
            Visible = true,
            Icon    = new System.Drawing.Icon(
                GetResourceStream(new Uri("Resources/tray_icon.ico", UriKind.Relative))?.Stream
                ?? throw new FileNotFoundException("tray_icon.ico not found"))
        };

        var menu = new System.Windows.Forms.ContextMenuStrip();

        var showItem = menu.Items.Add("Show MonitorSplit");
        showItem.Click += (_, _) =>
        {
            var w = _services?.GetRequiredService<MainWindow>();
            w?.Show();
            w?.Activate();
        };

        var enableItem = menu.Items.Add("Enable Split");
        enableItem.Click += (_, _) =>
        {
            var vm = _services?.GetRequiredService<MainViewModel>();
            vm?.EnableSplitCommand.Execute(null);
        };

        var disableItem = menu.Items.Add("Disable Split");
        disableItem.Click += (_, _) =>
        {
            var vm = _services?.GetRequiredService<MainViewModel>();
            vm?.DisableSplitCommand.Execute(null);
        };

        menu.Items.Add(new System.Windows.Forms.ToolStripSeparator());

        var exitItem = menu.Items.Add("Exit");
        exitItem.Click += (_, _) => Shutdown();

        _trayIcon.ContextMenuStrip = menu;
        _trayIcon.DoubleClick += (_, _) =>
        {
            var w = _services?.GetRequiredService<MainWindow>();
            w?.Show();
            w?.Activate();
        };
    }

    protected override void OnExit(ExitEventArgs e)
    {
        _trayIcon?.Dispose();
        _services?.GetService<DriverManager>()?.Dispose();
        _services?.Dispose();
        base.OnExit(e);
    }
}
