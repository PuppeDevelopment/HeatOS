param(
    [switch]$SkipIso,
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x86"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Get-IsExplorerLaunch {
    try {
        $self = Get-CimInstance Win32_Process -Filter "ProcessId = $PID"
        if (-not $self) { return $false }
        $parent = Get-Process -Id $self.ParentProcessId -ErrorAction SilentlyContinue
        return $null -ne $parent -and $parent.ProcessName -ieq "explorer"
    } catch { return $false }
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

# ---- Paths ----------------------------------------------------------------
$projectRoot  = Split-Path -Parent $PSScriptRoot
$buildDir     = Join-Path $projectRoot "build"
$srcDir       = Join-Path $projectRoot "src"
$includeDir   = Join-Path $srcDir "include"
$linkerScript = Join-Path $projectRoot $(if ($is64) { "kernel64.ld" } else { "kernel.ld" })

$bootSource   = Join-Path $srcDir "boot\boot.asm"
$entrySource  = Join-Path $srcDir $(if ($is64) { "kernel\entry64.asm" } else { "kernel\entry.asm" })

$bootBin      = Join-Path $buildDir ("boot{0}.bin" -f $artifactSuffix)
$kernelElf    = Join-Path $buildDir ("kernel{0}.elf" -f $artifactSuffix)
$kernelBin    = Join-Path $buildDir ("kernel{0}.bin" -f $artifactSuffix)
$imagePath    = Join-Path $buildDir ("Heatos{0}.img" -f $artifactSuffix)
$isoPath      = Join-Path $buildDir ("Heatos{0}.iso" -f $artifactSuffix)
$imageFileName = [System.IO.Path]::GetFileName($imagePath)

$maxKernelSectors = 256
$maxKernelBytes   = 512 * $maxKernelSectors
$floppyImageBytes = 1474560

New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

# ---- Find NASM ------------------------------------------------------------
$nasmPath = $null
$nasmCmd = Get-Command nasm -ErrorAction SilentlyContinue
if ($nasmCmd) { $nasmPath = $nasmCmd.Source }
else {
    $nasmPath = @(
        (Join-Path $env:ProgramFiles "NASM\nasm.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "NASM\nasm.exe")
    ) | Where-Object { $null -ne $_ -and (Test-Path $_) } | Select-Object -First 1
}
if (-not $nasmPath) {
    throw "NASM not found. Install: winget install --id NASM.NASM -e"
}
Write-Host "NASM: $nasmPath" -ForegroundColor DarkGray

# ---- Find Clang (LLVM) ----------------------------------------------------
$clangPath = $null
$clangCmd = Get-Command clang -ErrorAction SilentlyContinue
if ($clangCmd) { $clangPath = $clangCmd.Source }
else {
    $candidate = Join-Path $env:ProgramFiles "LLVM\bin\clang.exe"
    if (Test-Path $candidate) { $clangPath = $candidate }
}
if (-not $clangPath) {
    throw "Clang not found. Install: winget install --id LLVM.LLVM -e"
}
Write-Host "Clang: $clangPath" -ForegroundColor DarkGray

# ---- Find ld.lld and llvm-objcopy (same dir as clang) ---------------------
$llvmBinDir = Split-Path $clangPath
$lldPath    = Join-Path $llvmBinDir "ld.lld.exe"
$objcopyPath = Join-Path $llvmBinDir "llvm-objcopy.exe"

if (-not (Test-Path $lldPath))     { throw "ld.lld not found in $llvmBinDir" }
if (-not (Test-Path $objcopyPath)) { throw "llvm-objcopy not found in $llvmBinDir" }

# ---- Collect C/C++ source files -------------------------------------------
if ($is64) {
    $cSources = @((Get-Item (Join-Path $srcDir "kernel\main64.c")))
    $cppSources = @()
} else {
    $cSources = Get-ChildItem -Recurse -Filter "*.c" -Path $srcDir | Where-Object { $_.Name -ne "main64.c" }
    $cppSources = Get-ChildItem -Recurse -Filter "*.cpp" -Path $srcDir
}

Write-Host ""
Write-Host "=== HeatOS Build (C/C++ + ASM, arch=$Arch) ===" -ForegroundColor Cyan
Write-Host ""

# ---- Step 1: Assemble kernel entry (ELF32 object) -------------------------
if ($is64) {
    Write-Host "  [ASM64] entry64.asm" -ForegroundColor Yellow
    $entryObj = Join-Path $buildDir "entry64.o"
    $null = & $nasmPath -f elf64 $entrySource -o $entryObj 2>&1
    if ($LASTEXITCODE -ne 0) { throw "NASM failed on entry64.asm" }
} else {
    Write-Host "  [ASM] entry.asm, gdt_flush.asm, idt_flush.asm" -ForegroundColor Yellow
    $entryObj = Join-Path $buildDir "entry.o"
    $gdtFlushObj = Join-Path $buildDir "gdt_flush.o"
    $idtFlushObj = Join-Path $buildDir "idt_flush.o"
    $null = & $nasmPath -f elf32 $entrySource -o $entryObj 2>&1
    $null = & $nasmPath -f elf32 "src/kernel/gdt_flush.asm" -o $gdtFlushObj 2>&1
    $null = & $nasmPath -f elf32 "src/kernel/idt_flush.asm" -o $idtFlushObj 2>&1
    if ($LASTEXITCODE -ne 0) { throw "NASM failed on entry.asm" }
}

# ---- Step 2: Compile all C files ------------------------------------------
if ($is64) {
    $cFlags = @(
        "--target=x86_64-none-elf",
        "-march=x86-64",
        "-mno-red-zone",
        "-ffreestanding",
        "-nostdlib",
        "-fno-stack-protector",
        "-fno-pie",
        "-fno-pic",
        "-O2",
        "-Wall",
        "-Wextra",
        "-I$includeDir",
        "-c"
    )

    $cppFlags = @()
    $objFiles = @($entryObj)
} else {
    $cFlags = @(
        "--target=i386-none-elf",
        "-march=i386",
        "-mno-mmx",
        "-mno-sse",
        "-mno-sse2",
        "-msoft-float",
        "-fno-vectorize",
        "-fno-slp-vectorize",
        "-ffreestanding",
        "-nostdlib",
        "-fno-stack-protector",
        "-fno-pie",
        "-O2",
        "-Wall",
        "-Wextra",
        "-I$includeDir",
        "-c"
    )

    $cppFlags = @(
        "--target=i386-none-elf",
        "-march=i386",
        "-mno-mmx",
        "-mno-sse",
        "-mno-sse2",
        "-msoft-float",
        "-fno-vectorize",
        "-fno-slp-vectorize",
        "-ffreestanding",
        "-nostdlib",
        "-nostdlib++",
        "-fno-stack-protector",
        "-fno-pie",
        "-fno-exceptions",
        "-fno-rtti",
        "-fno-threadsafe-statics",
        "-fno-use-cxa-atexit",
        "-O2",
        "-Wall",
        "-Wextra",
        "-std=c++17",
        "-I$includeDir",
        "-c",
        "-x",
        "c++"
    )

    $objFiles = @($entryObj, $gdtFlushObj, $idtFlushObj)
}

foreach ($cs in $cSources) {
    $objName = [System.IO.Path]::GetFileNameWithoutExtension($cs.Name) + ".o"
    $objPath = Join-Path $buildDir $objName
    Write-Host "  [CC]  $($cs.Name)" -ForegroundColor Yellow
    $clangOutput = & $clangPath @cFlags $cs.FullName -o $objPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        $clangOutput | ForEach-Object { Write-Host $_ }
        throw "Clang failed on $($cs.Name)"
    }
    $objFiles += $objPath
}

foreach ($cpps in $cppSources) {
    $objName = [System.IO.Path]::GetFileNameWithoutExtension($cpps.Name) + ".cpp.o"
    $objPath = Join-Path $buildDir $objName
    Write-Host "  [CXX] $($cpps.Name)" -ForegroundColor Yellow
    $clangOutput = & $clangPath @cppFlags $cpps.FullName -o $objPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        $clangOutput | ForEach-Object { Write-Host $_ }
        throw "Clang failed on $($cpps.Name)"
    }
    $objFiles += $objPath
}

# ---- Step 3: Link into ELF ------------------------------------------------
Write-Host "  [LD]  $([System.IO.Path]::GetFileName($kernelElf))" -ForegroundColor Yellow
$lldOutput = & $lldPath -T $linkerScript -nostdlib -o $kernelElf @objFiles 2>&1
if ($LASTEXITCODE -ne 0) {
    $lldOutput | ForEach-Object { Write-Host $_ }
    throw "ld.lld failed to link kernel"
}

# ---- Step 4: Convert ELF to flat binary -----------------------------------
Write-Host "  [BIN] $([System.IO.Path]::GetFileName($kernelBin))" -ForegroundColor Yellow
$null = & $objcopyPath -O binary $kernelElf $kernelBin 2>&1
if ($LASTEXITCODE -ne 0) { throw "llvm-objcopy failed" }

# ---- Step 5: Calculate kernel size ----------------------------------------
$kernelBytes = [System.IO.File]::ReadAllBytes($kernelBin)
$kernelSectors = [int][Math]::Ceiling($kernelBytes.Length / 512.0)
if ($kernelSectors -lt 1) { $kernelSectors = 1 }

if ($kernelBytes.Length -gt $maxKernelBytes) {
    throw "Kernel too large ($($kernelBytes.Length) bytes). Max: $maxKernelBytes bytes."
}

# ---- Step 6: Assemble boot sector (with KERNEL_SECTORS) -------------------
Write-Host "  [ASM] boot.asm (sectors=$kernelSectors)" -ForegroundColor Yellow
$null = & $nasmPath -f bin $bootSource -o $bootBin "-DKERNEL_SECTORS=$kernelSectors" 2>&1
if ($LASTEXITCODE -ne 0) { throw "NASM failed on boot.asm" }

$bootBytes = [System.IO.File]::ReadAllBytes($bootBin)
if ($bootBytes.Length -ne 512) {
    throw "Boot sector must be 512 bytes (got $($bootBytes.Length))."
}

# ---- Step 7: Build floppy image -------------------------------------------
Write-Host "  [IMG] $imageFileName" -ForegroundColor Yellow
$disk = New-Object byte[] $floppyImageBytes
[System.Array]::Copy($bootBytes, 0, $disk, 0, $bootBytes.Length)
[System.Array]::Copy($kernelBytes, 0, $disk, 512, $kernelBytes.Length)
[System.IO.File]::WriteAllBytes($imagePath, $disk)

# ---- Optional: ISO ---------------------------------------------------------
if (-not $SkipIso) {
    $isoTool = $null
    foreach ($candidate in @("xorriso", "mkisofs", "genisoimage")) {
        $found = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($found) { $isoTool = $found; break }
    }
    if ($isoTool) {
        if (Test-Path $isoPath) { Remove-Item $isoPath -Force }
        if ($isoTool.Name -ieq "xorriso") {
            & $isoTool.Source -as mkisofs -quiet -V "HEATOS" -o $isoPath -b $imageFileName -c "boot.cat" $buildDir
        } else {
            & $isoTool.Source -quiet -V "HEATOS" -o $isoPath -b $imageFileName -c "boot.cat" $buildDir
        }
    } else {
        Write-Host "  ISO tool not found (skipping)." -ForegroundColor DarkGray
    }
}

# ---- Done ------------------------------------------------------------------
Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "  Boot:   $bootBin"
Write-Host "  Kernel: $kernelBin ($($kernelBytes.Length) bytes, $kernelSectors sectors)"
Write-Host "  Image:  $imagePath"
if (Test-Path $isoPath) { Write-Host "  ISO:    $isoPath" }

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
