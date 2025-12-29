@echo off
setlocal
rem ==============================================================================
rem EAIS TEST RUNNER (Windows) - Calls PowerShell Script
rem ==============================================================================

set SCRIPT_DIR=%~dp0
set PS_SCRIPT=%SCRIPT_DIR%..\Scripts\TestEAIS.ps1

echo [EAIS] Starting Automated Test Pipeline...

powershell.exe -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Pipeline FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Pipeline Complete!
endlocal
