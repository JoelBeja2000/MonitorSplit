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

## Fase 3: Scripts de Instalación
- [x] `install/Enable-TestSigning.ps1`
- [x] `install/Install-Driver.ps1`
- [x] `install/Uninstall-Driver.ps1`

## Fase 4: Documentación
- [x] `README.md`
- [x] `LICENSE` (MIT)
- [x] `docs/BUILD.md` — cómo compilar el driver
- [ ] `docs/ARCHITECTURE.md` — diagramas detallados
- [ ] `CONTRIBUTING.md`

## Pendiente (próximos pasos)
- [ ] Recursos de imagen (icon.ico, tray_icon.ico)
- [ ] app.manifest (elevación UAC para instalación)
- [ ] Tests unitarios para la app de control
- [ ] Pipeline CI/CD (GitHub Actions)
- [ ] Firma del driver (EV Certificate o SignPath)
