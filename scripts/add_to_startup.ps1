$WshShell = New-Object -ComObject WScript.Shell
$ShortcutPath = Join-Path ([Environment]::GetFolderPath("Startup")) "HeatOS.lnk"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TargetPath = Join-Path $ProjectRoot "run.cmd"

$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $TargetPath
$Shortcut.WorkingDirectory = $ProjectRoot
$Shortcut.Description = "Launch HeatOS on Windows Startup"
$Shortcut.Save()

Write-Host "HeatOS shortcut created in Windows Startup folder: $ShortcutPath" -ForegroundColor Green
