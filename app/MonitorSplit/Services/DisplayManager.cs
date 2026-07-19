// DisplayManager.cs — Reads physical monitor information from Windows
// Uses Win32 EnumDisplayMonitors and GetMonitorInfo to enumerate displays.

using System.Runtime.InteropServices;
using MonitorSplit.Models;

namespace MonitorSplit.Services;

/// <summary>
/// Reads information about physically connected monitors using Win32 APIs.
/// </summary>
public static class DisplayManager
{
    // ─── P/Invoke ────────────────────────────────────────────────────
    [DllImport("user32.dll")]
    private static extern bool EnumDisplayMonitors(
        IntPtr hdc, IntPtr lprcClip,
        MonitorEnumDelegate lpfnEnum, IntPtr dwData);

    private delegate bool MonitorEnumDelegate(
        IntPtr hMonitor, IntPtr hdcMonitor,
        ref RECT lprcMonitor, IntPtr dwData);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFOEX lpmi);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool EnumDisplayDevices(
        string? lpDevice, uint iDevNum,
        ref DISPLAY_DEVICE lpDisplayDevice, uint dwFlags);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool EnumDisplaySettings(
        string lpszDeviceName, int iModeNum,
        ref DEVMODE lpDevMode);

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct MONITORINFOEX
    {
        public uint    cbSize;
        public RECT    rcMonitor;
        public RECT    rcWork;
        public uint    dwFlags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  szDevice;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DISPLAY_DEVICE
    {
        public uint  cb;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string DeviceName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceString;
        public uint  StateFlags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceID;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceKey;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DEVMODE
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmDeviceName;
        public ushort dmSpecVersion;
        public ushort dmDriverVersion;
        public ushort dmSize;
        public ushort dmDriverExtra;
        public uint   dmFields;
        public int    dmPositionX;
        public int    dmPositionY;
        public uint   dmDisplayOrientation;
        public uint   dmDisplayFixedOutput;
        public short  dmColor;
        public short  dmDuplex;
        public short  dmYResolution;
        public short  dmTTOption;
        public short  dmCollate;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmFormName;
        public ushort dmLogPixels;
        public uint   dmBitsPerPel;
        public uint   dmPelsWidth;
        public uint   dmPelsHeight;
        public uint   dmDisplayFlags;
        public uint   dmDisplayFrequency;
    }

    private const uint MONITORINFOF_PRIMARY = 0x00000001;
    private const int  ENUM_CURRENT_SETTINGS = -1;

    // ─── Public API ──────────────────────────────────────────────────

    /// <summary>
    /// Returns all currently active physical monitors (excludes virtual monitors).
    /// </summary>
    public static List<PhysicalMonitor> GetPhysicalMonitors()
    {
        var monitors = new List<PhysicalMonitor>();

        EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero,
            (hMon, hdcMon, ref rect, data) =>
            {
                var mi = new MONITORINFOEX { cbSize = (uint)Marshal.SizeOf<MONITORINFOEX>() };
                if (!GetMonitorInfo(hMon, ref mi)) return true;

                var dm = new DEVMODE { dmSize = (ushort)Marshal.SizeOf<DEVMODE>() };
                EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, ref dm);

                // Get friendly name via EnumDisplayDevices
                string friendlyName = GetFriendlyName(mi.szDevice);

                // Skip the MonitorSplit virtual monitors (identified by manufacturer name)
                if (friendlyName.Contains("MSplit", StringComparison.OrdinalIgnoreCase))
                    return true;

                monitors.Add(new PhysicalMonitor
                {
                    DeviceName   = mi.szDevice,
                    FriendlyName = string.IsNullOrEmpty(friendlyName) ? mi.szDevice : friendlyName,
                    Width        = (int)dm.dmPelsWidth,
                    Height       = (int)dm.dmPelsHeight,
                    RefreshHz    = (int)dm.dmDisplayFrequency,
                    IsPrimary    = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0
                });

                return true;
            },
            IntPtr.Zero);

        return monitors;
    }

    private static string GetFriendlyName(string deviceName)
    {
        var dd = new DISPLAY_DEVICE { cb = (uint)Marshal.SizeOf<DISPLAY_DEVICE>() };
        uint i = 0;
        while (EnumDisplayDevices(deviceName, i++, ref dd, 0))
        {
            if (!string.IsNullOrWhiteSpace(dd.DeviceString))
                return dd.DeviceString;
        }
        return deviceName;
    }
}
