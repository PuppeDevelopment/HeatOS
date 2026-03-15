@echo off
setlocal

set "PS_EXE="
where pwsh.exe >nul 2>&1
if %ERRORLEVEL%==0 set "PS_EXE=pwsh.exe"
if not defined PS_EXE set "PS_EXE=powershell.exe"

"%PS_EXE%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build.ps1" %*
exit /b %ERRORLEVEL%