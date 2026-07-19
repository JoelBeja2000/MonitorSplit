#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Uninstall-Driver.ps1 — Removes the MonitorSplit driver from Windows.
#>

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Red
Write-Host "║           MonitorSplit — Driver Uninstaller              ║" -ForegroundColor Red
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Red
Write-Host ""

$confirm = Read-Host "This will remove the MonitorSplit driver and virtual monitors. Continue? (Y/N)"
if ($confirm -ne 'Y' -and $confirm -ne 'y') {
    Write-Host "Cancelled." -ForegroundColor Yellow
    exit 0
}

# Remove device nodes first
$devconLocations = @(
    "C:\Program Files (x86)\Windows Kits\10\Tools\x64\devcon.exe",
    "C:\Program Files (x86)\Windows Kits\11\Tools\x64\devcon.exe"
)
$devcon = $devconLocations | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($devcon) {
    Write-Host "Removing virtual device node..." -ForegroundColor Cyan
    & $devcon remove "Root\MonitorSplitDriver" 2>&1 | Write-Host -ForegroundColor Gray
}

# Remove from driver store
Write-Host "Removing driver from driver store..." -ForegroundColor Cyan
& pnputil /delete-driver "MonitorSplitDriver.inf" /uninstall 2>&1 | Write-Host -ForegroundColor Gray

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Driver removed successfully." -ForegroundColor Green
} else {
    Write-Host "⚠ Driver may have already been removed or was not found." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done. The virtual monitors will disappear after the next logon or reboot." -ForegroundColor Cyan
Write-Host ""
