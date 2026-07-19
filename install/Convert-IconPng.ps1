#Requires -Version 5.1
# Convert-IconPng.ps1 - Converts the PNG app icon to ICO format.
# Generates icon.ico and tray_icon.ico from a source PNG file.
# Uses System.Drawing to create multi-size ICO files (16, 32, 48, 256).

param(
    [string]$SourcePng = "$PSScriptRoot\..\docs\screenshot_mockup.png"
)

Add-Type -AssemblyName System.Drawing

function ConvertTo-Ico {
    param(
        [string]$InputPng,
        [string]$OutputIco,
        [int[]]$Sizes = @(16, 32, 48, 256)
    )

    Write-Host "Converting $InputPng -> $OutputIco ..." -ForegroundColor Cyan

    $srcBitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $InputPng).Path)

    $iconStream = New-Object System.IO.MemoryStream

    [byte[]]$header = @(0,0, 1,0, $Sizes.Count,0)
    $iconStream.Write($header, 0, $header.Length)

    $imageStreams = @()
    foreach ($size in $Sizes) {
        $scaled = New-Object System.Drawing.Bitmap($size, $size)
        $g = [System.Drawing.Graphics]::FromImage($scaled)
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.DrawImage($srcBitmap, 0, 0, $size, $size)
        $g.Dispose()

        $ms = New-Object System.IO.MemoryStream
        $scaled.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $imageStreams += $ms
        $scaled.Dispose()
    }

    $headerSize  = 6
    $entrySize   = 16
    $dirSize     = $headerSize + $entrySize * $Sizes.Count
    $currentOffset = $dirSize

    $iconStream.Seek($headerSize, [System.IO.SeekOrigin]::Begin) | Out-Null

    foreach ($i in 0..($Sizes.Count-1)) {
        $size      = $Sizes[$i]
        $dataSize  = $imageStreams[$i].Length
        $w         = if ($size -eq 256) { 0 } else { $size }
        $h         = if ($size -eq 256) { 0 } else { $size }

        [byte[]]$entry = @(
            $w, $h, 0, 0, 1, 0, 32, 0,
            [byte]($dataSize -band 0xFF),
            [byte](($dataSize -shr 8) -band 0xFF),
            [byte](($dataSize -shr 16) -band 0xFF),
            [byte](($dataSize -shr 24) -band 0xFF),
            [byte]($currentOffset -band 0xFF),
            [byte](($currentOffset -shr 8) -band 0xFF),
            [byte](($currentOffset -shr 16) -band 0xFF),
            [byte](($currentOffset -shr 24) -band 0xFF)
        )
        $iconStream.Write($entry, 0, $entry.Length)
        $currentOffset += $dataSize
    }

    foreach ($ms in $imageStreams) {
        $ms.Seek(0, [System.IO.SeekOrigin]::Begin) | Out-Null
        $ms.CopyTo($iconStream)
        $ms.Dispose()
    }

    $srcBitmap.Dispose()

    [System.IO.File]::WriteAllBytes($OutputIco, $iconStream.ToArray())
    $iconStream.Dispose()

    $sizeList = $Sizes -join ", "
    Write-Host "  OK: Written $OutputIco ($sizeList px)" -ForegroundColor Green
}

# Output directory
$resourcesDir = "$PSScriptRoot\..\app\MonitorSplit\Resources"
New-Item -ItemType Directory -Force -Path $resourcesDir | Out-Null

if (-not (Test-Path $SourcePng)) {
    Write-Host "ERROR: Source PNG not found: $SourcePng" -ForegroundColor Red
    exit 1
}

# Main app icon (all sizes)
ConvertTo-Ico -InputPng $SourcePng `
              -OutputIco "$resourcesDir\icon.ico" `
              -Sizes @(16, 32, 48, 64, 128, 256)

# Tray icon (small sizes only)
ConvertTo-Ico -InputPng $SourcePng `
              -OutputIco "$resourcesDir\tray_icon.ico" `
              -Sizes @(16, 32)

Write-Host ""
Write-Host "Done! Icon files generated in: $resourcesDir" -ForegroundColor Cyan
