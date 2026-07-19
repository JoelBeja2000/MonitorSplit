# Contributing to MonitorSplit

Thank you for your interest in contributing! MonitorSplit is an open-source project and we welcome contributions of all kinds.

## Ways to Contribute

- 🐛 **Bug reports** — Open an issue with steps to reproduce
- ✨ **Feature requests** — Describe your use case in an issue
- 🔧 **Code contributions** — Submit a pull request
- 📖 **Documentation** — Improve README, BUILD.md, ARCHITECTURE.md
- 🌐 **Translations** — Help localize the app UI

---

## Development Setup

### Prerequisites

| Tool | Where to get |
|------|-------------|
| Git | https://git-scm.com |
| Visual Studio 2022 (Community OK) | https://visualstudio.microsoft.com |
| WDK (Windows Driver Kit) | https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk |
| .NET 8 SDK | https://dotnet.microsoft.com/download/dotnet/8.0 |
| PowerShell 7+ | https://github.com/PowerShell/PowerShell |

### Clone and Build

```bash
git clone https://github.com/your-org/MonitorSplit.git
cd MonitorSplit
```

**Build the driver** (requires WDK):
```
Open driver\MonitorSplitDriver.vcxproj in Visual Studio 2022
Set platform: x64 | Release
Build → Build Solution
```

**Build the app**:
```powershell
cd app\MonitorSplit
dotnet build -c Debug
```

**Run tests**:
```powershell
cd app\MonitorSplit.Tests
dotnet test
```

---

## Project Structure

```
MonitorSplit/
├── driver/          C++ IDD driver (requires WDK to build)
├── app/             C# WPF control application
│   ├── MonitorSplit/          Main application
│   └── MonitorSplit.Tests/    xUnit unit tests
├── install/         PowerShell installation scripts
├── docs/            Developer documentation
└── .github/         CI/CD workflows
```

---

## Coding Standards

### C++ (Driver)

- Follow the **Microsoft IDD sample** conventions
- Use `TraceInfo()` / `TraceError()` for all logging (WPP tracing)
- All public functions must have a `//=====` comment block explaining purpose
- Never use raw `new`/`delete` — use WDF object lifetimes or RAII wrappers
- `NT_SUCCESS()` check **every** NTSTATUS return value

### C# (App)

- **MVVM strictly**: no Win32 P/Invoke calls from XAML code-behind
- Use `[ObservableProperty]` and `[RelayCommand]` from CommunityToolkit.Mvvm
- Services are injected via constructor DI — no `ServiceLocator` pattern
- Async methods must use `Async` suffix and return `Task<T>`
- Strings displayed to users should be in `Resources/*.resx` (not hardcoded)

---

## Pull Request Process

1. **Fork** the repository
2. **Create a branch**: `git checkout -b feature/my-feature` or `fix/issue-123`
3. **Make your changes** with clear, focused commits
4. **Write or update tests** for any new functionality
5. **Run tests**: `dotnet test`
6. **Update docs** if your change affects behavior
7. **Open a PR** against the `main` branch with:
   - A clear description of what changed and why
   - Screenshots/recordings if it's a UI change
   - Reference to any related issues (`Fixes #123`)

---

## Issues

When filing a bug report, please include:
- Windows version (`winver`)
- GPU model and driver version
- Whether Test Signing is enabled (`bcdedit /enum | findstr testsigning`)
- Steps to reproduce
- Expected vs actual behavior
- Device Manager screenshot showing the virtual monitors (or lack thereof)

---

## Architecture

See [docs/ARCHITECTURE.md](ARCHITECTURE.md) for a detailed explanation of how the driver and app work together.

---

## License

By contributing, you agree that your contributions will be licensed under the same [MIT License](../LICENSE) that covers the project.
