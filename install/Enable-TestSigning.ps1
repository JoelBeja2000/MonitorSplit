#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Enable-TestSigning.ps1 — Enables Windows Test Signing mode for MonitorSplit driver development.

.DESCRIPTION
    Enables bcdedit test signing so Windows will load unsigned drivers.
    REQUIRED for development. For production/distribution, use a signed driver instead.

.NOTES
    - Must be run as Administrator
    - Requires a system REBOOT to take effect
    - To disable test signing after you're done, run: bcdedit /set testsigning off
    
    ⚠ WARNING: Test signing mode disables some Windows security checks.
       Only use in development environments.
#>

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "|           MonitorSplit - Enable Test Signing             |" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Check current status
$status = & bcdedit /enum '{current}' 2>&1
$testSigningEnabled = $status -match 'testsigning\s+Yes'

if ($testSigningEnabled) {
    Write-Host "[o] Test signing is already ENABLED." -ForegroundColor Green
    Write-Host ""
    Write-Host "You can proceed to install the driver." -ForegroundColor White
} else {
    Write-Host "Enabling Test Signing mode..." -ForegroundColor Yellow
    
    $result = & bcdedit /set testsigning on 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "[o] Test Signing ENABLED successfully!" -ForegroundColor Green
        Write-Host ""
        Write-Host "[!] A SYSTEM REBOOT IS REQUIRED for this to take effect." -ForegroundColor Yellow
        Write-Host ""
        
        $reboot = Read-Host "Reboot now? (Y/N)"
        if ($reboot -eq 'Y' -or $reboot -eq 'y') {
            Write-Host "Rebooting in 5 seconds..." -ForegroundColor Yellow
            Start-Sleep -Seconds 5
            Restart-Computer -Force
        } else {
            Write-Host "Remember to reboot before installing the driver." -ForegroundColor Cyan
        }
    } else {
        Write-Host "[x] Failed to enable Test Signing:" -ForegroundColor Red
        Write-Host $result -ForegroundColor Red
        Write-Host ""
        Write-Host "Make sure Secure Boot is DISABLED in your BIOS/UEFI settings." -ForegroundColor Yellow
        exit 1
    }
}

Write-Host ""
Write-Host "Next step: Run Install-Driver.ps1 to install the MonitorSplit driver." -ForegroundColor Cyan
Write-Host ""
