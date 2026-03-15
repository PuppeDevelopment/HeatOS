param(
    [switch]$SkipBuild
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
    $imagePath = Join-Path $projectRoot "build\Heatos.img"

    if (-not $SkipBuild) {
        & (Join-Path $PSScriptRoot "build.ps1")

        if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
            throw "Build step failed. Fix the build error shown above, then run again."
        }
    }

    if (-not (Test-Path $imagePath)) {
        throw "Disk image not found: $imagePath"
    }

    $qemu = Get-Command qemu-system-i386 -ErrorAction SilentlyContinue
    if (-not $qemu) {
        $qemu = Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue
    }
    $qemuPath = $null

    if ($qemu) {
        $qemuPath = $qemu.Source
    }
    else {
        $qemuPath = @(
            (Join-Path $env:ProgramFiles "qemu\qemu-system-i386.exe"),
            (Join-Path $env:ProgramFiles "qemu\qemu-system-x86_64.exe"),
            (Join-Path $env:ProgramFiles "QEMU\qemu-system-i386.exe"),
            (Join-Path $env:ProgramFiles "QEMU\qemu-system-x86_64.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "qemu\qemu-system-i386.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "qemu\qemu-system-x86_64.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "QEMU\qemu-system-i386.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "QEMU\qemu-system-x86_64.exe")
        ) | Where-Object { $null -ne $_ -and (Test-Path $_) } | Select-Object -First 1
    }

    if (-not $qemuPath) {
        throw "qemu-system-i386 was not found. Install with: winget install --id SoftwareFreedomConservancy.QEMU -e --accept-package-agreements --accept-source-agreements, then reopen your terminal."
    }

    $global:LASTEXITCODE = 0
    & $qemuPath -drive "file=$imagePath,format=raw,if=floppy" -boot a -m 64M

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
