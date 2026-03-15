param(
    [switch]$SkipIso
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Get-IsExplorerLaunch {
    try {
        $self = Get-CimInstance Win32_Process -Filter "ProcessId = $PID"
        if (-not $self) {
            return $false
        }

        $parent = Get-Process -Id $self.ParentProcessId -ErrorAction SilentlyContinue
        return $null -ne $parent -and $parent.ProcessName -ieq "explorer"
    }
    catch {
        return $false
    }
}

$pauseOnExit = Get-IsExplorerLaunch

function Pause-IfNeeded {
    if ($pauseOnExit) {
        Write-Host ""
        Read-Host "Press Enter to close"
    }
}

try {

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot "build"

$bootSource = Join-Path $projectRoot "src\boot\boot.asm"
$kernelSource = Join-Path $projectRoot "src\kernel\kernel.asm"

$bootBin = Join-Path $buildDir "boot.bin"
$kernelBin = Join-Path $buildDir "kernel.bin"
$imagePath = Join-Path $buildDir "Heatos.img"
$isoPath = Join-Path $buildDir "Heatos.iso"

$maxKernelSectors = 128
$maxKernelBytes = 512 * $maxKernelSectors
$floppyImageBytes = 1474560

New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

$nasm = Get-Command nasm -ErrorAction SilentlyContinue
$nasmPath = $null

if ($nasm) {
    $nasmPath = $nasm.Source
}
else {
    $nasmPath = @(
        (Join-Path $env:ProgramFiles "NASM\nasm.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "NASM\nasm.exe")
    ) | Where-Object { $null -ne $_ -and (Test-Path $_) } | Select-Object -First 1
}

if (-not $nasmPath) {
    throw "NASM was not found. Install with: winget install --id NASM.NASM -e --accept-package-agreements --accept-source-agreements, then reopen your terminal."
}

& $nasmPath -f bin $kernelSource -o $kernelBin

$kernelBytes = [System.IO.File]::ReadAllBytes($kernelBin)
$kernelSectors = [int][Math]::Ceiling($kernelBytes.Length / 512.0)
if ($kernelSectors -lt 1) {
    $kernelSectors = 1
}

if ($kernelBytes.Length -gt $maxKernelBytes) {
    throw "Kernel is too large ($($kernelBytes.Length) bytes). Max is $maxKernelBytes bytes ($maxKernelSectors sectors)."
}

& $nasmPath -f bin $bootSource -o $bootBin "-DKERNEL_SECTORS=$kernelSectors"

$bootBytes = [System.IO.File]::ReadAllBytes($bootBin)
if ($bootBytes.Length -ne 512) {
    throw "Bootloader must be exactly 512 bytes. Current size: $($bootBytes.Length) bytes."
}

$disk = New-Object byte[] $floppyImageBytes
[System.Array]::Copy($bootBytes, 0, $disk, 0, $bootBytes.Length)
[System.Array]::Copy($kernelBytes, 0, $disk, 512, $kernelBytes.Length)

[System.IO.File]::WriteAllBytes($imagePath, $disk)

if (-not $SkipIso) {
    $isoTool = $null

    foreach ($candidate in @("xorriso", "mkisofs", "genisoimage")) {
        $found = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($found) {
            $isoTool = $found
            break
        }
    }

    if ($isoTool) {
        if (Test-Path $isoPath) {
            Remove-Item $isoPath -Force
        }

        if ($isoTool.Name -ieq "xorriso") {
            & $isoTool.Source -as mkisofs -quiet -V "HEATOS" -o $isoPath -b "Heatos.img" -c "boot.cat" $buildDir
        }
        else {
            & $isoTool.Source -quiet -V "HEATOS" -o $isoPath -b "Heatos.img" -c "boot.cat" $buildDir
        }
    }
    else {
        Write-Host "ISO tool not found (xorriso/mkisofs/genisoimage). Skipping ISO creation." -ForegroundColor Yellow
    }
}

Write-Host "Build successful."
Write-Host "Bootloader: $bootBin"
Write-Host "Kernel:     $kernelBin"
Write-Host "Sectors:    $kernelSectors"
Write-Host "Disk image: $imagePath"

if (Test-Path $isoPath) {
    Write-Host "ISO image:  $isoPath"
}
}
catch {
    Write-Host ""
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    Pause-IfNeeded
    $global:LASTEXITCODE = 1
    return
}

$global:LASTEXITCODE = 0
Pause-IfNeeded
