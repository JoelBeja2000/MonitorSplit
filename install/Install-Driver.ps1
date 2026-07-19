#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Install-Driver.ps1 — Installs the MonitorSplit IDD driver.

.DESCRIPTION
    Installs MonitorSplitDriver.inf using pnputil and creates the virtual
    device node that Windows uses to detect the virtual monitors.

.PARAMETER DriverPath
    Path to the directory containing MonitorSplitDriver.inf and .dll.
    Defaults to the 'driver' subdirectory next to this script.

.EXAMPLE
    .\Install-Driver.ps1
    .\Install-Driver.ps1 -DriverPath "C:\MyBuild\driver"
#>

param(
    [string]$DriverPath = ""
)

# ─── Setup ──────────────────────────────────────────────────────────────
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if ($DriverPath -eq "") {
    $DriverPath = Join-Path $ScriptDir "..\driver"
}

$DriverPath = Resolve-Path $DriverPath -ErrorAction SilentlyContinue
$InfFile    = Join-Path $DriverPath "MonitorSplitDriver.inf"

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║           MonitorSplit — Driver Installer                ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "Driver path: $DriverPath" -ForegroundColor Gray

# ─── Checks ─────────────────────────────────────────────────────────────
if (-not (Test-Path $InfFile)) {
    Write-Host "✗ INF file not found: $InfFile" -ForegroundColor Red
    Write-Host "  Build the driver first using Visual Studio 2022 + WDK." -ForegroundColor Yellow
    exit 1
}

# Check test signing (required for development)
$bootStatus = & bcdedit /enum '{current}' 2>&1 | Select-String 'testsigning'
if ($bootStatus -notmatch 'Yes') {
    Write-Host "⚠ Test Signing is NOT enabled." -ForegroundColor Yellow
    Write-Host "  Run Enable-TestSigning.ps1 first, then reboot." -ForegroundColor Yellow
    $continue = Read-Host "Continue anyway? (Y/N)"
    if ($continue -ne 'Y' -and $continue -ne 'y') { exit 1 }
}

# ─── Step 1: Add driver to driver store ────────────────────────────────
Write-Host ""
Write-Host "[1/3] Adding driver to Windows Driver Store..." -ForegroundColor Cyan

$addOutput = & pnputil /add-driver "$InfFile" /install 2>&1
Write-Host $addOutput -ForegroundColor Gray

if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 3010) {
    Write-Host "✗ pnputil /add-driver failed (exit code: $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Driver added to store" -ForegroundColor Green

# ─── Step 2: Create the virtual device node ────────────────────────────
Write-Host ""
Write-Host "[2/3] Creating virtual device node..." -ForegroundColor Cyan

# Look for devcon.exe
$devconLocations = @(
    "C:\Program Files (x86)\Windows Kits\10\Tools\x64\devcon.exe",
    "C:\Program Files (x86)\Windows Kits\11\Tools\x64\devcon.exe",
    (Join-Path $ScriptDir "devcon.exe"),
    (Join-Path $ScriptDir "..\tools\devcon.exe")
)

$devcon = $devconLocations | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($devcon) {
    Write-Host "Using devcon.exe: $devcon" -ForegroundColor Gray
    $devconOutput = & $devcon install "$InfFile" "Root\MonitorSplitDriver" 2>&1
    Write-Host $devconOutput -ForegroundColor Gray

    if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq 1) {
        Write-Host "✓ Virtual device node created" -ForegroundColor Green
    } else {
        Write-Host "⚠ devcon.exe returned exit code $LASTEXITCODE" -ForegroundColor Yellow
        Write-Host "  You may need to create the device node manually via Device Manager:" -ForegroundColor Yellow
        Write-Host "  → Action → Add Legacy Hardware → Manual → Display adapters → Have Disk" -ForegroundColor Yellow
    }
} else {
    Write-Host "⚠ devcon.exe not found." -ForegroundColor Yellow
    Write-Host "  Manual installation required:" -ForegroundColor Yellow
    Write-Host "  1. Open Device Manager (devmgmt.msc)" -ForegroundColor White
    Write-Host "  2. Action → Add Legacy Hardware" -ForegroundColor White
    Write-Host "  3. Manually install hardware not in a list" -ForegroundColor White
    Write-Host "  4. Display adapters → Have Disk → Browse to: $DriverPath" -ForegroundColor White
    Write-Host ""
    Write-Host "  Alternatively, download devcon.exe from the WDK Extras:" -ForegroundColor Yellow
    Write-Host "  https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon" -ForegroundColor Cyan
}

# ─── Step 3: Verify installation ──────────────────────────────────────
Write-Host ""
Write-Host "[3/3] Verifying installation..." -ForegroundColor Cyan

Start-Sleep -Seconds 2

$driverInfo = & pnputil /enum-drivers 2>&1 | Select-String -Pattern "MonitorSplit" -Context 2,2
if ($driverInfo) {
    Write-Host "✓ Driver found in driver store:" -ForegroundColor Green
    $driverInfo | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
} else {
    Write-Host "⚠ Driver not found in driver store yet. It may still be loading." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Installation complete!" -ForegroundColor Green
Write-Host ""
Write-Host "  Next steps:" -ForegroundColor White
Write-Host "  1. Open Display Settings (Win+P or Settings → Display)" -ForegroundColor White
Write-Host "  2. You should see 2 new 'MonitorSplit' virtual monitors" -ForegroundColor White
Write-Host "  3. Launch MonitorSplit.exe to configure the split" -ForegroundColor White
Write-Host ""
Write-Host "  If monitors don't appear, try rebooting." -ForegroundColor Yellow
Write-Host "════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
