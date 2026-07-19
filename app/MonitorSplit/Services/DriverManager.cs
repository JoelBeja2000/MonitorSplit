// DriverManager.cs — Communication with the MonitorSplit Windows driver
// Handles driver installation, removal, and IOCTL communication.

using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using MonitorSplit.Models;

namespace MonitorSplit.Services;

/// <summary>
/// Manages installation and communication with the MonitorSplit IDD driver.
/// Uses P/Invoke to call Win32 SetupAPI and DeviceIoControl.
/// </summary>
public class DriverManager
{
    // ─── P/Invoke: Win32 SetupAPI / Kernel32 ────────────────────────
    private const string SETUPAPI_DLL = "setupapi.dll";
    private const string KERNEL32_DLL = "kernel32.dll";

    [DllImport(KERNEL32_DLL, SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr CreateFile(
        string lpFileName,
        uint dwDesiredAccess,
        uint dwShareMode,
        IntPtr lpSecurityAttributes,
        uint dwCreationDisposition,
        uint dwFlagsAndAttributes,
        IntPtr hTemplateFile);

    [DllImport(KERNEL32_DLL, SetLastError = true)]
    private static extern bool DeviceIoControl(
        IntPtr hDevice,
        uint dwIoControlCode,
        IntPtr lpInBuffer,
        uint nInBufferSize,
        IntPtr lpOutBuffer,
        uint nOutBufferSize,
        out uint lpBytesReturned,
        IntPtr lpOverlapped);

    [DllImport(KERNEL32_DLL, SetLastError = true)]
    private static extern bool CloseHandle(IntPtr hObject);

    private const uint GENERIC_READ   = 0x80000000;
    private const uint GENERIC_WRITE  = 0x40000000;
    private const uint OPEN_EXISTING  = 3;
    private const uint FILE_SHARE_READ  = 0x00000001;
    private const uint FILE_SHARE_WRITE = 0x00000002;
    private static readonly IntPtr INVALID_HANDLE = new IntPtr(-1);

    // ─── IOCTL codes (must match driver/MonitorSplitDriver.h) ───────
    private const uint FILE_DEVICE_VIDEO = 0x00000023;
    private const uint METHOD_BUFFERED   = 0;
    private const uint FILE_ANY_ACCESS   = 0;
    private const uint IOCTL_BASE        = 0x8000;

    private static uint CTL_CODE(uint devType, uint func, uint method, uint access) =>
        (devType << 16) | (access << 14) | (func << 2) | method;

    private static readonly uint IOCTL_SET_CONFIG = CTL_CODE(FILE_DEVICE_VIDEO, IOCTL_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS);
    private static readonly uint IOCTL_GET_CONFIG = CTL_CODE(FILE_DEVICE_VIDEO, IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS);
    private static readonly uint IOCTL_ENABLE     = CTL_CODE(FILE_DEVICE_VIDEO, IOCTL_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS);
    private static readonly uint IOCTL_DISABLE    = CTL_CODE(FILE_DEVICE_VIDEO, IOCTL_BASE + 4, METHOD_BUFFERED, FILE_ANY_ACCESS);

    // ─── Device interface GUID (from driver's INF / GUID definition) ─
    // {A1B2C3D4-E5F6-7890-ABCD-EF1234567890} — matches driver GUID
    private const string DEVICE_INTERFACE_GUID = "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}";

    // Symbolic link exposed by the driver
    private const string DEVICE_SYMBOLIC_LINK = @"\\.\MonitorSplitDriver";

    // ─── State ──────────────────────────────────────────────────────
    private IntPtr _deviceHandle = INVALID_HANDLE;

    public bool IsDriverInstalled => CheckDriverInstalled();
    public bool IsDriverConnected => _deviceHandle != INVALID_HANDLE;

    // ─── Driver Installation ─────────────────────────────────────────

    /// <summary>
    /// Checks whether the driver is currently installed (visible in Device Manager).
    /// </summary>
    private bool CheckDriverInstalled()
    {
        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(
                @"SYSTEM\CurrentControlSet\Services\MonitorSplitDriver");
            return key != null;
        }
        catch { return false; }
    }

    /// <summary>
    /// Installs the driver using pnputil. Requires Administrator elevation.
    /// </summary>
    /// <param name="infPath">Full path to MonitorSplitDriver.inf</param>
    public async Task<(bool Success, string Message)> InstallDriverAsync(string infPath)
    {
        if (!File.Exists(infPath))
            return (false, $"INF file not found: {infPath}");

        try
        {
            // Step 1: Add driver to driver store
            var addResult = await RunProcessAsync(
                "pnputil",
                $"/add-driver \"{infPath}\" /install");

            if (addResult.ExitCode != 0 && addResult.ExitCode != 3010)
                return (false, $"pnputil /add-driver failed (exit {addResult.ExitCode}):\n{addResult.Output}");

            // Step 2: Create the root-enumerated device node
            var devconPath = FindDevcon();
            if (devconPath != null)
            {
                var devconResult = await RunProcessAsync(
                    devconPath,
                    "install \"" + infPath + "\" Root\\MonitorSplitDriver");
                if (devconResult.ExitCode != 0)
                    return (false, $"devcon install failed:\n{devconResult.Output}");
            }
            else
            {
                // Fallback: use cfgmgr32 / SetupAPI approach
                return (false,
                    "devcon.exe not found. Please run the Install-Driver.ps1 script as Administrator, " +
                    "or install devcon.exe from the WDK extras.");
            }

            return (true, "Driver installed successfully. A reboot may be required.");
        }
        catch (Exception ex)
        {
            return (false, $"Installation failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Uninstalls the driver using pnputil. Requires Administrator elevation.
    /// </summary>
    public async Task<(bool Success, string Message)> UninstallDriverAsync()
    {
        try
        {
            var result = await RunProcessAsync(
                "pnputil",
                "/delete-driver MonitorSplitDriver.inf /uninstall");

            return result.ExitCode == 0
                ? (true, "Driver uninstalled successfully.")
                : (false, $"Uninstall failed (exit {result.ExitCode}):\n{result.Output}");
        }
        catch (Exception ex)
        {
            return (false, $"Uninstall failed: {ex.Message}");
        }
    }

    // ─── Device Connection ────────────────────────────────────────────

    /// <summary>
    /// Opens a handle to the driver device for IOCTL communication.
    /// </summary>
    public bool Connect()
    {
        if (_deviceHandle != INVALID_HANDLE) return true;

        _deviceHandle = CreateFile(
            DEVICE_SYMBOLIC_LINK,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            IntPtr.Zero,
            OPEN_EXISTING,
            0,
            IntPtr.Zero);

        if (_deviceHandle == INVALID_HANDLE)
        {
            var error = Marshal.GetLastWin32Error();
            System.Diagnostics.Debug.WriteLine($"CreateFile failed: error {error}");
            return false;
        }

        return true;
    }

    /// <summary>
    /// Closes the handle to the driver device.
    /// </summary>
    public void Disconnect()
    {
        if (_deviceHandle != INVALID_HANDLE)
        {
            CloseHandle(_deviceHandle);
            _deviceHandle = INVALID_HANDLE;
        }
    }

    // ─── IOCTL Communication ──────────────────────────────────────────

    /// <summary>
    /// Sends the split configuration to the driver via IOCTL.
    /// </summary>
    public bool SendConfig(SplitConfig config)
    {
        if (!IsDriverConnected && !Connect()) return false;

        var native = new NativeConfig
        {
            Version           = 1,
            Monitor1Width     = (uint)config.Monitor1Width,
            Monitor1Height    = (uint)config.Monitor1Height,
            Monitor1RefreshHz = (uint)config.Monitor1RefreshHz,
            Monitor2Width     = (uint)config.Monitor2Width,
            Monitor2Height    = (uint)config.Monitor2Height,
            Monitor2RefreshHz = (uint)config.Monitor2RefreshHz,
            IsEnabled         = config.IsEnabled ? 1u : 0u
        };

        int size = Marshal.SizeOf<NativeConfig>();
        IntPtr pBuf = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(native, pBuf, false);
            return DeviceIoControl(
                _deviceHandle, IOCTL_SET_CONFIG,
                pBuf, (uint)size,
                IntPtr.Zero, 0,
                out _, IntPtr.Zero);
        }
        finally
        {
            Marshal.FreeHGlobal(pBuf);
        }
    }

    /// <summary>
    /// Reads the current configuration from the driver.
    /// </summary>
    public SplitConfig? ReadConfig()
    {
        if (!IsDriverConnected && !Connect()) return null;

        int size = Marshal.SizeOf<NativeConfig>();
        IntPtr pBuf = Marshal.AllocHGlobal(size);
        try
        {
            bool ok = DeviceIoControl(
                _deviceHandle, IOCTL_GET_CONFIG,
                IntPtr.Zero, 0,
                pBuf, (uint)size,
                out uint bytesReturned, IntPtr.Zero);

            if (!ok || bytesReturned < size) return null;

            var native = Marshal.PtrToStructure<NativeConfig>(pBuf);
            return new SplitConfig
            {
                Version           = (int)native.Version,
                Monitor1Width     = (int)native.Monitor1Width,
                Monitor1Height    = (int)native.Monitor1Height,
                Monitor1RefreshHz = (int)native.Monitor1RefreshHz,
                Monitor2Width     = (int)native.Monitor2Width,
                Monitor2Height    = (int)native.Monitor2Height,
                Monitor2RefreshHz = (int)native.Monitor2RefreshHz,
                IsEnabled         = native.IsEnabled != 0
            };
        }
        finally
        {
            Marshal.FreeHGlobal(pBuf);
        }
    }

    /// <summary>Sends IOCTL to enable the virtual monitors.</summary>
    public bool Enable()
    {
        if (!IsDriverConnected && !Connect()) return false;
        return DeviceIoControl(_deviceHandle, IOCTL_ENABLE, IntPtr.Zero, 0, IntPtr.Zero, 0, out _, IntPtr.Zero);
    }

    /// <summary>Sends IOCTL to disable the virtual monitors.</summary>
    public bool Disable()
    {
        if (!IsDriverConnected && !Connect()) return false;
        return DeviceIoControl(_deviceHandle, IOCTL_DISABLE, IntPtr.Zero, 0, IntPtr.Zero, 0, out _, IntPtr.Zero);
    }

    // ─── Native struct (must match MONITORSPLIT_CONFIG in the driver) ─
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    private struct NativeConfig
    {
        public uint Version;
        public uint Monitor1Width;
        public uint Monitor1Height;
        public uint Monitor1RefreshHz;
        public uint Monitor2Width;
        public uint Monitor2Height;
        public uint Monitor2RefreshHz;
        public uint IsEnabled;
    }

    // ─── Helpers ─────────────────────────────────────────────────────

    private record ProcessResult(int ExitCode, string Output);

    private static async Task<ProcessResult> RunProcessAsync(string exe, string args)
    {
        var psi = new System.Diagnostics.ProcessStartInfo(exe, args)
        {
            RedirectStandardOutput = true,
            RedirectStandardError  = true,
            UseShellExecute        = false,
            CreateNoWindow         = true
        };

        using var proc = System.Diagnostics.Process.Start(psi)
            ?? throw new InvalidOperationException($"Failed to start {exe}");

        string output = await proc.StandardOutput.ReadToEndAsync()
                      + await proc.StandardError.ReadToEndAsync();
        await proc.WaitForExitAsync();
        return new ProcessResult(proc.ExitCode, output);
    }

    private static string? FindDevcon()
    {
        // devcon.exe is part of the WDK extras — check common locations
        string[] candidates = [
            @"C:\Program Files (x86)\Windows Kits\10\Tools\x64\devcon.exe",
            @"C:\Program Files (x86)\Windows Kits\11\Tools\x64\devcon.exe",
            Path.Combine(AppContext.BaseDirectory, "devcon.exe")
        ];

        return candidates.FirstOrDefault(File.Exists);
    }

    public void Dispose() => Disconnect();
}
