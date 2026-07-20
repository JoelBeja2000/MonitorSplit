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
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "|           MonitorSplit - Driver Installer                |" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Driver path: $DriverPath" -ForegroundColor Gray

# ─── Checks ─────────────────────────────────────────────────────────────
if (-not (Test-Path $InfFile)) {
    Write-Host "[Error] INF file not found: $InfFile" -ForegroundColor Red
    Write-Host "  Build the driver first using Visual Studio 2022 + WDK." -ForegroundColor Yellow
    exit 1
}

# Check test signing (required for development)
$bootStatus = & bcdedit /enum '{current}' 2>&1 | Select-String 'testsigning'
if ($bootStatus -notmatch 'Yes') {
    Write-Host "[!] Test Signing is NOT enabled." -ForegroundColor Yellow
    Write-Host "  Run Enable-TestSigning.ps1 first, then reboot." -ForegroundColor Yellow
    $continue = Read-Host "Continue anyway? (Y/N)"
    if ($continue -ne 'Y' -and $continue -ne 'y') { exit 1 }
}

# --- Step 0: Sign driver (test-signing) --------------------------------
Write-Host ""
Write-Host "[0/3] Signing the driver package..." -ForegroundColor Cyan
& (Join-Path $ScriptDir "Sign-Driver.ps1")
if ($LASTEXITCODE -ne 0) {
    Write-Host "[x] Failed to sign the driver." -ForegroundColor Red
    exit 1
}

# --- Step 1: Full cleanup of all old device nodes + driver packages ----
Write-Host ""
Write-Host "[1/3] Full cleanup: removing ALL MonitorSplit device nodes and driver packages..." -ForegroundColor Cyan

# First, remove ALL MonitorSplit device nodes using devcon
$devconLocations = @(
    "C:\Program Files (x86)\Windows Kits\10\Tools\x64\devcon.exe",
    "C:\Program Files (x86)\Windows Kits\11\Tools\x64\devcon.exe",
    (Join-Path $ScriptDir "devcon.exe"),
    (Join-Path $ScriptDir "..\tools\devcon.exe")
)
$devcon = $devconLocations | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($devcon) {
    Write-Host "Removing all MonitorSplit device nodes..." -ForegroundColor Yellow
    & $devcon remove "Root\MonitorSplitDriver" 2>&1 | Out-Null
    Start-Sleep -Seconds 1
}

# Delete the old Windows service entry if present
Stop-Service -Name "MonitorSplitDriver" -Force -ErrorAction SilentlyContinue
sc.exe delete MonitorSplitDriver 2>&1 | Out-Null

# Remove all OEM driver packages via DISM / Get-WindowsDriver
try {
    $staleDrivers = Get-WindowsDriver -Online -ErrorAction SilentlyContinue | Where-Object { $_.ProviderName -match "MonitorSplit" -or $_.OriginalFileName -match "monitorsplit" }
    foreach ($drv in $staleDrivers) {
        Write-Host "Removing driver package $($drv.Driver)..." -ForegroundColor Yellow
        & pnputil /delete-driver $drv.Driver /uninstall /force 2>&1 | Out-Null
    }
} catch {}

# Fallback pnputil check
$oemDrivers = & pnputil /enum-drivers 2>&1 | Select-String -Pattern "oem\d+\.inf" -Context 0, 5
$existingOEMs = @()
foreach ($line in $oemDrivers) {
    if ($line.Line -match 'oem\d+\.inf') {
        $oemName = $Matches[0]
        $isMonitorSplit = $false
        foreach ($ctxLine in $line.Context.PostContext) {
            if ($ctxLine -match 'MonitorSplit') { $isMonitorSplit = $true }
        }
        if ($isMonitorSplit -and $existingOEMs -notcontains $oemName) {
            $existingOEMs += $oemName
        }
    }
}

foreach ($oem in $existingOEMs) {
    Write-Host "Removing driver package: $oem..." -ForegroundColor Yellow
    & pnputil /delete-driver $oem /uninstall /force 2>&1 | Out-Null
}

Write-Host "[o] Cleanup complete" -ForegroundColor Green

# Now add fresh driver
Write-Host "Adding fresh driver to store..." -ForegroundColor Cyan
$addOutput = & pnputil /add-driver "$InfFile" /install 2>&1
Write-Host $addOutput -ForegroundColor Gray

if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 3010 -and $LASTEXITCODE -ne 259) {
    Write-Host "[x] pnputil /add-driver failed (exit code: $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

Write-Host "[o] Driver added to store" -ForegroundColor Green

# --- Step 2: Ensure virtual device node is created and active ----------
Write-Host ""
Write-Host "[2/3] Activating virtual device node..." -ForegroundColor Cyan

if ($devcon) {
    Write-Host "Rescanning PnP device tree..." -ForegroundColor Gray
    & $devcon rescan 2>&1 | Out-Null
    
    # Check if device node is bound
    $device = Get-PnpDevice | Where-Object { $_.FriendlyName -match "MonitorSplit" -or $_.InstanceId -match "MonitorSplit" } | Select-Object -First 1
    if (-not $device) {
        Write-Host "Attempting driver update binding for Root\MonitorSplitDriver..." -ForegroundColor Cyan
        $updateOutput = & $devcon update "$InfFile" "Root\MonitorSplitDriver" 2>&1
        Write-Host $updateOutput -ForegroundColor Gray
    }
} else {
    Write-Host "[!] devcon.exe not found." -ForegroundColor Yellow
}

# Final check of device status
Start-Sleep -Seconds 2
$deviceStatus = Get-PnpDevice | Where-Object { $_.FriendlyName -match "MonitorSplit" -or $_.InstanceId -match "MonitorSplit" } | Select-Object Status, Problem, FriendlyName, InstanceId -First 1
if ($deviceStatus) {
    Write-Host "[o] Virtual Device found: $($deviceStatus.InstanceId) - Status: $($deviceStatus.Status)" -ForegroundColor Green
} else {
    Write-Host "[!] Virtual device node created. Rebooting may be required for Windows to initialize the display adapter." -ForegroundColor Yellow
}

# --- Step 3: Verify installation --------------------------------------
Write-Host ""
Write-Host "[3/3] Verifying installation..." -ForegroundColor Cyan

Start-Sleep -Seconds 2

$driverInfo = & pnputil /enum-drivers 2>&1 | Select-String -Pattern "MonitorSplit" -Context 2,2
if ($driverInfo) {
    Write-Host "[o] Driver found in driver store:" -ForegroundColor Green
    $driverInfo | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
} else {
    Write-Host "[!] Driver not found in driver store yet. It may still be loading." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Installation complete!" -ForegroundColor Green
Write-Host ""
Write-Host "  Next steps:" -ForegroundColor White
Write-Host "  1. Open Display Settings (Win+P or Settings -> Display)" -ForegroundColor White
Write-Host "  2. You should see 2 new 'MonitorSplit' virtual monitors" -ForegroundColor White
Write-Host "  3. Launch MonitorSplit.exe to configure the split" -ForegroundColor White
Write-Host ""
Write-Host "  If monitors don't appear, try rebooting." -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
