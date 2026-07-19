# Building MonitorSplit from Source

## Prerequisites

### 1. Visual Studio 2022

Download **Visual Studio 2022 Community** (free):  
https://visualstudio.microsoft.com/downloads/

Required workloads:
- **Desktop development with C++**
- **.NET desktop development**

### 2. Windows Driver Kit (WDK)

The WDK version must match your Visual Studio version.

1. Download from: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
2. Install the **Windows SDK** first (usually included with VS)
3. Then install the **WDK** (it integrates into Visual Studio)

> [!IMPORTANT]
> Make sure to install the WDK that matches your VS version. As of 2024, WDK 10.0.26100 works with VS 2022.

### 3. .NET 8 SDK

Download from: https://dotnet.microsoft.com/download/dotnet/8.0

---

## Building the Driver (C++)

1. Open Visual Studio 2022
2. File → Open → Project → `driver\MonitorSplitDriver.vcxproj`
3. Select **Release | x64** in the toolbar
4. Build → Build Solution (or Ctrl+Shift+B)

**Output** (in `bin\Release\x64\driver\`):
- `MonitorSplitDriver.dll` — The driver binary
- `MonitorSplitDriver.inf` — Installation descriptor  
- `MonitorSplitDriver.cat` — Catalog file (requires signing for production)

### Compiler errors?

**"Cannot open include file: 'wdf.h'"**  
→ WDK is not properly installed. Re-run the WDK installer.

**"IddCx.lib: No such file or directory"**  
→ Your WDK version may be too old. IddCx requires WDK 10.0.17134+.

**"UMDF_USING_NTSTATUS: macro redefined"**  
→ Add `UMDF_USING_NTSTATUS` only once in the preprocessor definitions.

---

## Building the Control App (C#)

```powershell
cd app\MonitorSplit

# Debug build
dotnet build

# Release build (self-contained single-file executable)
dotnet publish -c Release -r win-x64 --self-contained true
```

Output: `app\MonitorSplit\bin\Release\net8.0-windows\win-x64\publish\MonitorSplit.exe`

---

## Signing the Driver (for Production)

For public distribution without Test Signing mode, the driver must be:

1. **Code-signed** with an EV (Extended Validation) Certificate (~$300-500/year)
2. Optionally submitted to **Microsoft's WHQL** for hardware certification

### Test Signing (Development Only)

```powershell
# Enable test signing (run as admin, then reboot)
bcdedit /set testsigning on
```

After reboot, a watermark "Test Mode" will appear in the bottom-right of the desktop.

### Using signtool (if you have a certificate)

```powershell
# Sign the driver DLL
signtool sign /fd SHA256 /ac "CrossCertificate.crt" /f "MyCert.pfx" /p "password" `
              /t "http://timestamp.digicert.com" `
              "bin\Release\x64\driver\MonitorSplitDriver.dll"

# Generate the catalog
inf2cat /driver:"bin\Release\x64\driver" /os:10_X64,Server2019_X64

# Sign the catalog
signtool sign /fd SHA256 /ac "CrossCertificate.crt" /f "MyCert.pfx" /p "password" `
              /t "http://timestamp.digicert.com" `
              "bin\Release\x64\driver\MonitorSplitDriver.cat"
```

---

## Project Structure (Developer Notes)

### Driver Callbacks Flow

```
DriverEntry()
  └─ WdfDriverCreate()
       └─ EvtDriverDeviceAdd()
            ├─ IddCxDeviceInitConfig()
            ├─ IddCxDeviceInitialize()  ← registers all IDD callbacks
            └─ WdfDeviceCreate()
                 └─ EvtDeviceD0Entry() → EvtAdapterInitFinished()
                                              └─ CreateMonitors(×2)
                                                   └─ IddCxMonitorArrival()
                                                        └─ EvtMonitorAssignSwapChain()
                                                             └─ SwapChainThread()
```

### Adding More Virtual Monitors

Change `MONITORSPLIT_MONITOR_COUNT` in `MonitorSplitDriver.h` from `2` to your desired count, and update the `MONITORSPLIT_CONFIG` struct accordingly.

### Changing Default Resolution

Edit the `DEFAULT_MONITOR*` constants in `MonitorSplitDriver.h`:

```cpp
#define DEFAULT_MONITOR1_WIDTH    1280  // e.g. for a 2560-wide ultrawide
#define DEFAULT_MONITOR1_HEIGHT   1440
#define DEFAULT_MONITOR2_WIDTH    1280
#define DEFAULT_MONITOR2_HEIGHT   1440
```
