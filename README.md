
# MonitorSplit

> **Split your physical monitor into two virtual monitors that Windows detects as independent displays.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows 10/11](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078d4?logo=windows)](https://www.microsoft.com/windows)
[![Driver: IddCx (UMDF2)](https://img.shields.io/badge/Driver-IddCx%20UMDF2-7C5CFC)](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview)

![MonitorSplit Screenshot](docs/screenshot.png)

---

## What it does

MonitorSplit creates **two virtual monitors** that Windows (and all applications) treat as independent physical displays. Unlike zone-management tools like PowerToys FancyZones, MonitorSplit uses a **real Windows Indirect Display Driver (IDD)** — which means:

- ✅ Each virtual monitor has its own **resolution and refresh rate**
- ✅ Full-screen applications can use **each half independently**
- ✅ Windows **taskbar spans** both virtual monitors (if configured)
- ✅ Streaming software (OBS, Sunshine, etc.) sees them as **separate captures**
- ✅ Works with **any physical monitor** — ultrawide, standard 16:9, etc.

---

## Architecture

```
Physical Monitor (e.g. 1920×1080)
        │
        ├── MonitorSplit IDD Driver (UMDF2 / IddCx)
        │       │
        │       ├── Virtual Monitor 1: 960×1080 @ 60Hz
        │       └── Virtual Monitor 2: 960×1080 @ 60Hz
        │
        └── MonitorSplit.exe (GUI Control App)
                 ├── Configure split ratio (10%–90%)
                 ├── Set resolution per half
                 ├── Install/uninstall driver
                 └── System tray icon
```

---

## Requirements

| Component | Requirement |
|-----------|------------|
| OS | Windows 10 version 2004 (20H1) or later, Windows 11 |
| Architecture | x64 (AMD64) |
| .NET | .NET 8 Runtime (for the GUI app) |
| Driver signing | Test signing enabled **OR** EV code-signed driver |

---

## Quick Start

### Option A — Development (Test Signing)

1. **Enable Test Signing** (run as Administrator, then reboot):
   ```powershell
   .\install\Enable-TestSigning.ps1
   ```

2. **Build the driver** (see [docs/BUILD.md](docs/BUILD.md)):
   - Requires Visual Studio 2022 + WDK 10.0.26100+
   - Open `driver\MonitorSplitDriver.vcxproj` → Build → x64/Release

3. **Install the driver** (run as Administrator):
   ```powershell
   .\install\Install-Driver.ps1
   ```

4. **Launch the app**:
   ```
   app\MonitorSplit\bin\Release\net8.0-windows\win-x64\MonitorSplit.exe
   ```

5. In the app: adjust the split slider → click **Activate Split**

6. Open **Settings → System → Display** — you should see 2 new monitors!

---

### Option B — Pre-built Release

1. Download the latest release from [Releases](https://github.com/your-org/MonitorSplit/releases)
2. Extract the ZIP
3. Run `install\Enable-TestSigning.ps1` as Administrator (reboot if needed)
4. Run `install\Install-Driver.ps1` as Administrator
5. Run `MonitorSplit.exe`

---

## Configuration

The app stores its configuration in:
```
%APPDATA%\MonitorSplit\config.json
```

Example `config.json`:
```json
{
  "Version": 1,
  "Monitor1Width": 1280,
  "Monitor1Height": 1080,
  "Monitor1RefreshHz": 60,
  "Monitor2Width": 640,
  "Monitor2Height": 1080,
  "Monitor2RefreshHz": 60,
  "SplitRatio": 0.67,
  "IsEnabled": true
}
```

---

## Project Structure

```
MonitorSplit/
├── driver/                      ← Windows IDD Driver (C++)
│   ├── MonitorSplitDriver.cpp   ← Main driver implementation
│   ├── MonitorSplitDriver.h     ← Types, IOCTLs, function prototypes
│   ├── Edid.h                   ← Custom EDID generator
│   ├── Trace.h                  ← WPP tracing macros
│   ├── MonitorSplitDriver.inf   ← Driver installation descriptor
│   └── MonitorSplitDriver.vcxproj ← VS 2022 project file
│
├── app/MonitorSplit/            ← Control App (C# WPF / .NET 8)
│   ├── Models/SplitConfig.cs   ← Configuration model
│   ├── Services/
│   │   ├── DriverManager.cs     ← IOCTL + install/uninstall
│   │   └── DisplayManager.cs   ← Enumerate physical monitors
│   ├── ViewModels/MainViewModel.cs ← MVVM ViewModel
│   ├── Themes/                  ← Dark mode styles
│   ├── Converters/              ← WPF value converters
│   ├── MainWindow.xaml/.cs      ← Main UI
│   └── App.xaml/.cs             ← Startup + tray icon
│
├── install/                     ← PowerShell scripts
│   ├── Enable-TestSigning.ps1
│   ├── Install-Driver.ps1
│   └── Uninstall-Driver.ps1
│
└── docs/                        ← Documentation
    ├── BUILD.md
    └── ARCHITECTURE.md
```

---

## FAQ

**Q: Will this work without Test Signing?**  
A: No. For production use, the driver .cat file must be signed with an EV Code Signing Certificate and submitted to Microsoft's WHQL (optional but recommended for hardware IDs).

**Q: Does it affect my physical monitor?**  
A: No. The driver creates *additional* virtual monitors. Your physical monitor continues to work normally. You organize all monitors in Windows Display Settings.

**Q: Will games work in full-screen on the virtual monitors?**  
A: Yes for DirectX games using borderless window mode. Exclusive full-screen may revert to the physical monitor depending on the game engine.

**Q: Is it safe?**  
A: The driver uses UMDF2 (User Mode Driver Framework), which is the modern, safe approach — it runs in user mode, not kernel mode, so a driver crash cannot BSOD your system.

**Q: Does it survive reboots?**  
A: Yes. Once installed, the driver loads automatically with Windows. The virtual monitors reappear after reboot.

---

## Building from Source

See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions.

**TL;DR:**
```powershell
# Driver (requires Visual Studio 2022 + WDK)
# Open driver\MonitorSplitDriver.vcxproj in Visual Studio → Build

# App
cd app\MonitorSplit
dotnet build -c Release
```

---

## Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

Key areas where help is appreciated:
- 🔑 **Code signing** — Set up CI/CD with SignPath or similar for signed binaries
- 🎨 **UI improvements** — Better monitor arrangement visualization
- 🌐 **Internationalization** — Multi-language support
- 🧪 **Testing** — Automated tests for the control app

---

## License

MIT License — see [LICENSE](LICENSE) for details.

Based on the [Microsoft Windows Driver Samples](https://github.com/microsoft/Windows-driver-samples) 
(also MIT Licensed) IddSampleDriver.
