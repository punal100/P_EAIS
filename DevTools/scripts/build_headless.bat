@echo off
setlocal
rem ==============================================================================
rem HEADLESS BUILD SCRIPT
rem ==============================================================================

rem Path to Unreal Editor (can be overridden via UE_ENGINE_ROOT env var)
if defined UE_ENGINE_ROOT (
    set UE_ROOT=%UE_ENGINE_ROOT%
) else (
    set UE_ROOT=d:\UE\UE_S
)

set RUNUAT_CMD="%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"

rem Find .uproject file (4 levels up from scripts/)
set SEARCH_DIR=%~dp0..\..\..\..
for %%f in ("%SEARCH_DIR%\*.uproject") do (
    set UPROJECT_PATH="%%f"
    goto :found_project
)
echo [EAIS] ERROR: No .uproject file found
exit /b 1

:found_project
echo [EAIS] Building Project (Includes Plugin)...
echo [EAIS] Project: %UPROJECT_PATH%

call %RUNUAT_CMD% BuildCookRun -project=%UPROJECT_PATH% -noP4 -clientconfig=Development -serverconfig=Development -platform=Win64 -build -unattended -utf8output

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Build FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Build SUCCESS!
endlocal
pause
