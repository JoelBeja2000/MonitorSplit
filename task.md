# MonitorSplit — Task List

## Fase 1: Driver IDD (C++)
- [x] Crear estructura de directorios del proyecto
- [x] `driver/MonitorSplitDriver.cpp` — implementación IDD principal
- [x] `driver/MonitorSplitDriver.h` — cabeceras, IOCTLs, tipos
- [x] `driver/Edid.h` — generación EDID personalizado (128 bytes)
- [x] `driver/Trace.h` — macros WPP tracing
- [x] `driver/MonitorSplitDriver.inf` — archivo de instalación del driver
- [x] `driver/MonitorSplitDriver.vcxproj` — proyecto Visual Studio para el driver

## Fase 2: Aplicación de Control (C# WPF)
- [x] `app/MonitorSplit.sln` — solución Visual Studio (driver + app)
- [x] `app/MonitorSplit/MonitorSplit.csproj` — proyecto WPF .NET 8
- [x] `app/MonitorSplit/App.xaml` + `.cs` — startup, DI, tray icon
- [x] `app/MonitorSplit/MainWindow.xaml` — UI principal (dark mode moderno)
- [x] `app/MonitorSplit/MainWindow.xaml.cs` — code-behind
- [x] `app/MonitorSplit/Models/SplitConfig.cs`
- [x] `app/MonitorSplit/Services/DriverManager.cs`
- [x] `app/MonitorSplit/Services/DisplayManager.cs`
- [x] `app/MonitorSplit/ViewModels/MainViewModel.cs`
- [x] `app/MonitorSplit/Converters/Converters.cs`
- [x] `app/MonitorSplit/Themes/Colors.xaml` — sistema de colores dark mode
- [x] `app/MonitorSplit/Themes/Controls.xaml` — estilos de controles
- [x] `app/MonitorSplit/app.manifest` — DPI awareness + compatibilidad Windows 10/11
- [x] `app/MonitorSplit/Resources/icon.ico` — generado desde PNG (16,32,48,64,128,256px)
- [x] `app/MonitorSplit/Resources/tray_icon.ico` — generado desde PNG (16,32px)

## Fase 3: Tests Unitarios
- [x] `app/MonitorSplit.Tests/MonitorSplit.Tests.csproj` — proyecto xUnit
- [x] `app/MonitorSplit.Tests/SplitConfigTests.cs` — tests del modelo
- [x] `app/MonitorSplit.Tests/MainViewModelTests.cs` — tests del ViewModel

## Fase 4: Scripts de Instalación
- [x] `install/Enable-TestSigning.ps1`
- [x] `install/Install-Driver.ps1`
- [x] `install/Uninstall-Driver.ps1`
- [x] `install/Convert-IconPng.ps1` — convierte PNG a ICO

## Fase 5: CI/CD y DevOps
- [x] `.github/workflows/ci.yml` — GitHub Actions (build, test, release)
- [x] `.gitignore`
- [x] `git init` + primer commit (38 archivos, 4842 líneas)

## Fase 6: Documentación
- [x] `README.md`
- [x] `LICENSE` (MIT)
- [x] `docs/BUILD.md` — cómo compilar el driver
- [x] `docs/ARCHITECTURE.md` — diagramas detallados con secuencias
- [x] `CONTRIBUTING.md`
- [x] `docs/screenshot_mockup.png` — icono/mockup del proyecto

## COMPLETADO - Próximos pasos para el usuario
- [ ] Instalar Visual Studio 2022 + WDK y compilar el driver
- [ ] Habilitar Test Signing y ejecutar Install-Driver.ps1
- [ ] Crear repositorio en GitHub y hacer `git remote add origin ...`
- [ ] Firmar el driver con EV Certificate para distribución pública (opcional)
