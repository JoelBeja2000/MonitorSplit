// SplitConfig.cs — Configuration model for MonitorSplit
// Represents the user's chosen split settings.

namespace MonitorSplit.Models;

/// <summary>
/// Configuration for the virtual monitor split.
/// Serialized to JSON in AppData for persistence across sessions.
/// Also sent to the driver via IOCTL.
/// </summary>
public class SplitConfig
{
    public const int CurrentVersion = 1;

    public int Version { get; set; } = CurrentVersion;

    // ─── Monitor 1 (Left half) ──────────────────────────────────────
    public int Monitor1Width { get; set; } = 960;
    public int Monitor1Height { get; set; } = 1080;
    public int Monitor1RefreshHz { get; set; } = 60;

    // ─── Monitor 2 (Right half) ─────────────────────────────────────
    public int Monitor2Width { get; set; } = 960;
    public int Monitor2Height { get; set; } = 1080;
    public int Monitor2RefreshHz { get; set; } = 60;

    // ─── Split Ratio ────────────────────────────────────────────────
    /// <summary>
    /// Percentage of the physical monitor width given to Monitor 1 (0.0 – 1.0).
    /// Default: 0.5 (50/50 split)
    /// </summary>
    public double SplitRatio { get; set; } = 0.5;

    // ─── Source Monitor ─────────────────────────────────────────────
    /// <summary>
    /// The name of the physical monitor to base the split on.
    /// Used for display purposes only — Windows handles the actual layout.
    /// </summary>
    public string? PhysicalMonitorName { get; set; }

    public int PhysicalWidth { get; set; } = 1920;
    public int PhysicalHeight { get; set; } = 1080;

    // ─── State ──────────────────────────────────────────────────────
    public bool IsEnabled { get; set; } = false;
    public bool StartWithWindows { get; set; } = false;

    /// <summary>
    /// Compute Monitor1Width from physical width and split ratio.
    /// </summary>
    public void RecalculateFromPhysical()
    {
        if (PhysicalHeight > PhysicalWidth)
        {
            // Portrait mode: split top and bottom (vertical stack)
            Monitor1Width  = PhysicalWidth;
            Monitor1Height = (int)(PhysicalHeight * SplitRatio);
            Monitor2Width  = PhysicalWidth;
            Monitor2Height = PhysicalHeight - Monitor1Height;
        }
        else
        {
            // Landscape mode: split left and right (horizontal side-by-side)
            Monitor1Width  = (int)(PhysicalWidth * SplitRatio);
            Monitor1Height = PhysicalHeight;
            Monitor2Width  = PhysicalWidth - Monitor1Width;
            Monitor2Height = PhysicalHeight;
        }
    }
}

/// <summary>
/// Represents a detected physical monitor with its display properties.
/// </summary>
public class PhysicalMonitor
{
    public string DeviceName { get; set; } = string.Empty;
    public string FriendlyName { get; set; } = string.Empty;
    public int Width { get; set; }
    public int Height { get; set; }
    public int RefreshHz { get; set; }
    public bool IsPrimary { get; set; }

    public override string ToString() =>
        $"{FriendlyName} ({Width}×{Height} @ {RefreshHz}Hz){(IsPrimary ? " [Primary]" : "")}";
}
