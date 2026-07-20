// MainViewModel.cs — ViewModel for the main window (MVVM pattern)
// Uses CommunityToolkit.Mvvm for observable properties and commands.

using System.Collections.ObjectModel;
using System.IO;
using System.Text.Json;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MonitorSplit.Models;
using MonitorSplit.Services;

namespace MonitorSplit.ViewModels;

/// <summary>
/// Main ViewModel: bridges the UI and the driver/services.
/// </summary>
public partial class MainViewModel : ObservableObject
{
    private readonly DriverManager _driverManager;
    private readonly string _configPath;

    public MainViewModel(DriverManager driverManager)
    {
        _driverManager = driverManager;
        _configPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "MonitorSplit", "config.json");

        LoadConfig();
        RefreshMonitors();
        RefreshDriverStatus();
        UpdatePreviewDimensions();
    }

    // ─── Observable Properties ────────────────────────────────────────

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsPortrait))]
    [NotifyPropertyChangedFor(nameof(SplitRatioPercent))]
    [NotifyPropertyChangedFor(nameof(SplitRatioSlider))]
    private SplitConfig _config = new();

    [ObservableProperty]
    private ObservableCollection<PhysicalMonitor> _physicalMonitors = [];

    [ObservableProperty]
    private PhysicalMonitor? _selectedMonitor;

    [ObservableProperty]
    private bool _isDriverInstalled;

    [ObservableProperty]
    private bool _isDriverConnected;

    [ObservableProperty]
    private string _statusMessage = "Ready";

    [ObservableProperty]
    private bool _isBusy;

    [ObservableProperty]
    private double _previewWidth = 400;

    [ObservableProperty]
    private double _previewHeight = 225;

    // ─── Computed Properties ──────────────────────────────────────────

    /// <summary>Whether the currently selected physical monitor is vertical (portrait mode)</summary>
    public bool IsPortrait => Config.PhysicalHeight > Config.PhysicalWidth;

    /// <summary>Split ratio as percentage string (e.g. "50%")</summary>
    public string SplitRatioPercent => $"{Config.SplitRatio * 100:F0}%";

    // ─── Commands ─────────────────────────────────────────────────────

    [RelayCommand]
    private void RefreshMonitors()
    {
        PhysicalMonitors.Clear();
        foreach (var m in DisplayManager.GetPhysicalMonitors())
            PhysicalMonitors.Add(m);

        if (PhysicalMonitors.Count > 0 && SelectedMonitor == null)
        {
            SelectedMonitor = PhysicalMonitors.FirstOrDefault(m => m.IsPrimary)
                           ?? PhysicalMonitors[0];
            ApplySelectedMonitor();
        }

        StatusMessage = $"Found {PhysicalMonitors.Count} physical monitor(s)";
    }

    [RelayCommand]
    private void ApplySelectedMonitor()
    {
        if (SelectedMonitor == null) return;

        Config.PhysicalMonitorName = SelectedMonitor.FriendlyName;
        Config.PhysicalWidth       = SelectedMonitor.Width;
        Config.PhysicalHeight      = SelectedMonitor.Height;
        Config.Monitor1RefreshHz   = SelectedMonitor.RefreshHz;
        Config.Monitor2RefreshHz   = SelectedMonitor.RefreshHz;
        Config.RecalculateFromPhysical();
        UpdatePreviewDimensions();

        OnPropertyChanged(nameof(Config));
        OnPropertyChanged(nameof(SplitRatioPercent));
        OnPropertyChanged(nameof(IsPortrait));
    }

    partial void OnSelectedMonitorChanged(PhysicalMonitor? value)
    {
        if (value != null) ApplySelectedMonitor();
    }

    [RelayCommand]
    private async Task InstallDriverAsync()
    {
        IsBusy = true;
        StatusMessage = "Installing driver...";

        // Look for the INF next to the executable
        var infPath = Path.Combine(AppContext.BaseDirectory, "driver", "MonitorSplitDriver.inf");
        var (success, message) = await _driverManager.InstallDriverAsync(infPath);

        StatusMessage = message;
        IsBusy = false;
        RefreshDriverStatus();

        if (success)
        {
            System.Windows.MessageBox.Show(
                message,
                "MonitorSplit — Driver Installed",
                System.Windows.MessageBoxButton.OK,
                System.Windows.MessageBoxImage.Information);
        }
        else
        {
            System.Windows.MessageBox.Show(
                message,
                "MonitorSplit — Installation Failed",
                System.Windows.MessageBoxButton.OK,
                System.Windows.MessageBoxImage.Error);
        }
    }

    [RelayCommand]
    private async Task UninstallDriverAsync()
    {
        IsBusy = true;
        StatusMessage = "Uninstalling driver...";
        var (success, message) = await _driverManager.UninstallDriverAsync();
        StatusMessage = message;
        IsBusy = false;
        RefreshDriverStatus();
    }

    [RelayCommand]
    private void ToggleSplit()
    {
        if (Config.IsEnabled)
            DisableSplit();
        else
            EnableSplit();
    }

    [RelayCommand]
    private void EnableSplit()
    {
        if (!IsDriverInstalled)
        {
            StatusMessage = "Driver not installed. Please install the driver first.";
            return;
        }

        SaveConfig();
        bool ok = _driverManager.SendConfig(Config) && _driverManager.Enable();
        Config.IsEnabled = ok;
        StatusMessage = ok ? "✓ Split activated — two virtual monitors created" : "Failed to enable split";
        OnPropertyChanged(nameof(Config));
    }

    [RelayCommand]
    private void DisableSplit()
    {
        bool ok = _driverManager.Disable();
        Config.IsEnabled = false;
        StatusMessage = ok ? "Split deactivated" : "Failed to disable split";
        OnPropertyChanged(nameof(Config));
    }

    [RelayCommand]
    private void ApplyConfig()
    {
        if (!IsDriverInstalled) { StatusMessage = "Driver not installed"; return; }
        Config.RecalculateFromPhysical();
        bool ok = _driverManager.SendConfig(Config);
        StatusMessage = ok ? "✓ Configuration applied" : "Failed to apply configuration";
        SaveConfig();
        OnPropertyChanged(nameof(Config));
        OnPropertyChanged(nameof(SplitRatioPercent));
    }

    // ─── Split Ratio slider helper ─────────────────────────────────────

    public double SplitRatioSlider
    {
        get => Config.SplitRatio * 100;
        set
        {
            Config.SplitRatio = value / 100.0;
            Config.RecalculateFromPhysical();
            OnPropertyChanged(nameof(SplitRatioSlider));
            OnPropertyChanged(nameof(SplitRatioPercent));
            OnPropertyChanged(nameof(Config));
        }
    }

    // ─── Persistence ─────────────────────────────────────────────────

    private void SaveConfig()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_configPath)!);
            var json = JsonSerializer.Serialize(Config, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(_configPath, json);
        }
        catch (Exception ex)
        {
            StatusMessage = $"Warning: Could not save config: {ex.Message}";
        }
    }

    private void LoadConfig()
    {
        try
        {
            if (File.Exists(_configPath))
            {
                var json = File.ReadAllText(_configPath);
                var loaded = JsonSerializer.Deserialize<SplitConfig>(json);
                if (loaded != null)
                {
                    Config = loaded;
                    UpdatePreviewDimensions();
                }
            }
        }
        catch { /* Use defaults */ }
    }

    private void RefreshDriverStatus()
    {
        IsDriverInstalled = _driverManager.IsDriverInstalled;
        IsDriverConnected = _driverManager.IsDriverConnected;
    }

    private void UpdatePreviewDimensions()
    {
        double physicalWidth = Config.PhysicalWidth > 0 ? Config.PhysicalWidth : 1920;
        double physicalHeight = Config.PhysicalHeight > 0 ? Config.PhysicalHeight : 1080;
        double aspectRatio = physicalWidth / physicalHeight;

        // Bounding box limits for the preview area (max width 440, max height 240)
        const double maxW = 440;
        const double maxH = 240;

        if (aspectRatio > (maxW / maxH))
        {
            // Limited by width
            PreviewWidth = maxW;
            PreviewHeight = maxW / aspectRatio;
        }
        else
        {
            // Limited by height
            PreviewHeight = maxH;
            PreviewWidth = maxH * aspectRatio;
        }
    }
}
