param([switch]$DryRun)
$root    = Split-Path $PSScriptRoot -Parent
$srcFile = Join-Path $root "src\kernel\kernel_monolith.bak"
if (-not (Test-Path $srcFile)) { $srcFile = Join-Path $root "src\kernel\kernel.asm" }
Write-Host "Source: $srcFile"
$lines = [System.IO.File]::ReadAllLines($srcFile, [System.Text.Encoding]::UTF8)

function Write-Module([string]$relPath, [int[]]$ranges) {
    $fullPath = Join-Path $root $relPath
    $dir = Split-Path $fullPath
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    $output = New-Object System.Collections.ArrayList
    for ($i = 0; $i -lt $ranges.Count; $i += 2) {
        $s = $ranges[$i] - 1; $e = $ranges[$i+1] - 1
        if ($e -ge $lines.Count) { $e = $lines.Count - 1 }
        foreach ($ln in $lines[$s..$e]) { [void]$output.Add($ln) }
    }
    $content = ($output.ToArray() -join [System.Environment]::NewLine)
    if ($DryRun) { Write-Host "DryRun: $relPath ($($output.Count) lines)" }
    else {
        [System.IO.File]::WriteAllText($fullPath, $content, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host "  wrote $relPath ($($output.Count) lines)"
    }
}

Write-Host "=== Splitting kernel into modular source files ==="
Write-Module "src\drivers\mouse.asm"      @(2130, 2299)
Write-Module "src\drivers\pci_net.asm"    @(2300, 2468)
Write-Module "src\lib\video.asm"          @(2776, 2887, 3069, 3071)
Write-Module "src\lib\string.asm"         @(2647, 2660, 2888, 3029)
Write-Module "src\lib\input.asm"          @(2771, 2775, 3030, 3068, 3109, 3169)
Write-Module "src\lib\print.asm"          @(3072, 3108, 3170, 3217)
Write-Module "src\lib\math.asm"           @(2661, 2770, 3337, 3433)
Write-Module "src\lib\time.asm"           @(2077, 2129, 2469, 2646, 3218, 3336)
Write-Module "src\desktop\desktop.asm"    @(47, 280)
Write-Module "src\desktop\kickoff.asm"    @(281, 466)
Write-Module "src\desktop\run_dialog.asm" @(467, 576)
Write-Module "src\desktop\renderer.asm"   @(719, 1081, 1408, 1510)
Write-Module "src\apps\system.asm"        @(577, 599, 1082, 1165)
Write-Module "src\apps\files.asm"         @(600, 618, 1166, 1200)
Write-Module "src\apps\notes.asm"         @(619, 637, 1201, 1232)
Write-Module "src\apps\clock.asm"         @(638, 660, 1233, 1279)
Write-Module "src\apps\network.asm"       @(661, 687, 1280, 1351)
Write-Module "src\apps\power.asm"         @(688, 718, 1352, 1407)
Write-Module "src\terminal\terminal.asm"  @(1511, 1727)
Write-Module "src\terminal\commands.asm"  @(1728, 2076)
$dataStart = 0; $varStart = 0
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($dataStart -eq 0 -and $lines[$i] -match '^banner_msg\s+db') { $dataStart = $i + 1 }
    if ($varStart  -eq 0 -and $lines[$i] -match '^boot_drive\s+db') { $varStart  = $i + 1 }
}
if ($dataStart -eq 0) { $dataStart = 3434 }
if ($varStart  -eq 0) { $varStart  = 3719 }
Write-Host "  Data: strings=$dataStart  variables=$varStart  total=$($lines.Count)"
Write-Module "src\data\strings.asm"   @($dataStart, ($varStart - 1))
Write-Module "src\data\variables.asm" @($varStart, $lines.Count)
Write-Host "=== Done. Run build.cmd to test. ==="