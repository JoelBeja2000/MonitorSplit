#Requires -RunAsAdministrator
# Sign-Driver.ps1 - Generates a test certificate and signs the MonitorSplit driver.

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "|           MonitorSplit - Driver Signer Tool              |" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DriverDir = Resolve-Path (Join-Path $ScriptDir "..\driver")
$InfFile = Join-Path $DriverDir "MonitorSplitDriver.inf"
$DllFile = Join-Path $DriverDir "MonitorSplitDriver.dll"

# Paths to Windows Kit tools
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
$inf2cat = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe"

if (-not (Test-Path $signtool)) {
    Write-Host "[x] signtool.exe not found at: $signtool" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path $inf2cat)) {
    Write-Host "[x] Inf2Cat.exe not found at: $inf2cat" -ForegroundColor Red
    exit 1
}

# 1. Create or retrieve test certificate
$certName = "MonitorSplitTestCert"
$certStorePath = "Cert:\LocalMachine\My"
$cert = Get-ChildItem -Path $certStorePath | Where-Object { $_.Subject -match "CN=$certName" } | Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating self-signed test code-signing certificate..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate -Type CodeSigning -Subject "CN=$certName" -CertStoreLocation $certStorePath
    Write-Host "[o] Certificate created." -ForegroundColor Green
} else {
    Write-Host "[o] Certificate already exists." -ForegroundColor Green
}

# 2. Add certificate to Trusted Roots and Publishers
Write-Host "Adding certificate to Trusted Root and Trusted Publisher stores..." -ForegroundColor Yellow

$rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
$rootStore.Open("ReadWrite")
$rootStore.Add($cert)
$rootStore.Close()

$pubStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPublisher", "LocalMachine")
$pubStore.Open("ReadWrite")
$pubStore.Add($cert)
$pubStore.Close()

Write-Host "[o] Certificate registered as trusted on this PC." -ForegroundColor Green

# 3. Create .cat catalog file from .inf
Write-Host "Generating catalog file (MonitorSplitDriver.cat) using Inf2Cat..." -ForegroundColor Yellow
$inf2catOutput = & $inf2cat /driver:"$DriverDir" /os:10_x64 2>&1
Write-Host $inf2catOutput -ForegroundColor Gray

$catFile = Join-Path $DriverDir "MonitorSplitDriver.cat"
if (-not (Test-Path $catFile)) {
    Write-Host "[x] Inf2Cat failed to generate the catalog file." -ForegroundColor Red
    exit 1
}
Write-Host "[o] Catalog file created successfully." -ForegroundColor Green

# 4. Sign the DLL and CAT files
Write-Host "Signing driver files using signtool..." -ForegroundColor Yellow
$signDllOutput = & $signtool sign /a /sm /s My /n "$certName" /fd SHA256 /t "http://timestamp.digicert.com" "$DllFile" 2>&1
Write-Host $signDllOutput -ForegroundColor Gray

$signCatOutput = & $signtool sign /a /sm /s My /n "$certName" /fd SHA256 /t "http://timestamp.digicert.com" "$catFile" 2>&1
Write-Host $signCatOutput -ForegroundColor Gray

Write-Host "[o] Driver files signed successfully!" -ForegroundColor Green
Write-Host ""
