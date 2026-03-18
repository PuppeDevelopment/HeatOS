param(
    [switch]$SkipBuild,
    [switch]$BootIso,
    [switch]$NoNetwork,
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x86",
    [ValidateSet("virtio", "bga")]
    [string]$DisplayBackend = "virtio"
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
    $is64 = $Arch -ieq "x64"
    $artifactSuffix = if ($is64) { "64" } else { "" }

    $projectRoot = Split-Path -Parent $PSScriptRoot
    $imagePath = Join-Path $projectRoot ("build\Heatos{0}.img" -f $artifactSuffix)
    $isoPath = Join-Path $projectRoot ("build\Heatos{0}.iso" -f $artifactSuffix)

    if (-not $SkipBuild) {
        & (Join-Path $PSScriptRoot "build.ps1") -Arch $Arch

        if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
            throw "Build step failed. Fix the build error shown above, then run again."
        }
    }

    if ($BootIso) {
        if (-not (Test-Path $isoPath)) {
            throw "ISO image not found: $isoPath. Build with an ISO tool available (xorriso/mkisofs/genisoimage) or run without -BootIso."
        }
    }
    elseif (-not (Test-Path $imagePath)) {
        throw "Disk image not found: $imagePath"
    }

    $qemu = $null
    if ($is64) {
        $qemu = Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue
        if (-not $qemu) { $qemu = Get-Command qemu-system-i386 -ErrorAction SilentlyContinue }
    } else {
        $qemu = Get-Command qemu-system-i386 -ErrorAction SilentlyContinue
        if (-not $qemu) { $qemu = Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue }
    }
    $qemuPath = $null

    if ($qemu) {
        $qemuPath = $qemu.Source
    }
    else {
        $primary = if ($is64) { "x86_64" } else { "i386" }
        $secondary = if ($is64) { "i386" } else { "x86_64" }

        $qemuPath = @(
            (Join-Path $env:ProgramFiles ("qemu\qemu-system-{0}.exe" -f $primary)),
            (Join-Path $env:ProgramFiles ("QEMU\qemu-system-{0}.exe" -f $primary)),
            (Join-Path ${env:ProgramFiles(x86)} ("qemu\qemu-system-{0}.exe" -f $primary)),
            (Join-Path ${env:ProgramFiles(x86)} ("QEMU\qemu-system-{0}.exe" -f $primary)),
            (Join-Path $env:ProgramFiles ("qemu\qemu-system-{0}.exe" -f $secondary)),
            (Join-Path $env:ProgramFiles ("QEMU\qemu-system-{0}.exe" -f $secondary)),
            (Join-Path ${env:ProgramFiles(x86)} ("qemu\qemu-system-{0}.exe" -f $secondary)),
            (Join-Path ${env:ProgramFiles(x86)} ("QEMU\qemu-system-{0}.exe" -f $secondary))
        ) | Where-Object { $null -ne $_ -and (Test-Path $_) } | Select-Object -First 1
    }

    if (-not $qemuPath) {
        throw "QEMU was not found. Install with: winget install --id SoftwareFreedomConservancy.QEMU -e --accept-package-agreements --accept-source-agreements, then reopen your terminal."
    }

    $qemuArgs = @()

    if ($BootIso) {
        $qemuArgs += @("-cdrom", $isoPath, "-boot", "d")
    }
    else {
        $qemuArgs += @("-drive", "file=$imagePath,format=raw,if=floppy", "-boot", "a")
    }

    $qemuArgs += @("-m", "64M")

    if ($DisplayBackend -ieq "virtio") {
        $qemuArgs += @("-vga", "none", "-device", "virtio-vga,max_outputs=1")
    }

    if (-not $NoNetwork) {
        $qemuArgs += @("-nic", "user,model=ne2k_pci")
    }

    $global:LASTEXITCODE = 0
    & $qemuPath @qemuArgs

    if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
        throw "QEMU exited with code $LASTEXITCODE."
    }
}
catch {
    Write-Host ""
    Write-Host "Run failed: $($_.Exception.Message)" -ForegroundColor Red
    Pause-IfNeeded
    exit 1
}

Pause-IfNeeded
