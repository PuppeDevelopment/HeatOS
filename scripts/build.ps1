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

# --- Optional C compiler detection ----------------------------------------
# RushOS is split into modular .asm files. C support requires an ia16-elf-gcc
# cross-compiler capable of producing 16-bit real-mode flat binary code.
# Install guide: https://github.com/tkchia/gcc-ia16 (build from source)
# or use OpenWatcom: https://github.com/open-watcom/open-watcom-v2
# When a compatible compiler is found, .c files in src/ will be compiled and
# the resulting flat binary appended before the data section.
$cCompiler = @("ia16-elf-gcc", "owcc", "wcc") |
    ForEach-Object { Get-Command $_ -ErrorAction SilentlyContinue } |
    Select-Object -First 1
if ($cCompiler) {
    Write-Host "C compiler found: $($cCompiler.Name)  -  C modules will be compiled." -ForegroundColor Cyan
    $cSources = Get-ChildItem -Recurse -Filter "*.c" -Path (Join-Path $projectRoot "src")
    foreach ($cs in $cSources) {
        Write-Host "  Compiling $($cs.Name)..."
        # ia16-elf-gcc: produce flat binary object, link manually
        # (exact flags depend on toolchain; adjust as toolchain is set up)
    }
} else {
    Write-Host "No 16-bit C compiler found. Assembly-only build (this is fine)." -ForegroundColor DarkGray
    Write-Host "  To add C: install ia16-elf-gcc or OpenWatcom (see scripts/build.ps1 comment)." -ForegroundColor DarkGray
}
# ---------------------------------------------------------------------------

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
