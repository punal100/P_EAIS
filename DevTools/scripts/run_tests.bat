@echo off
setlocal
rem ==============================================================================
rem HEADLESS TEST RUNNER
rem ==============================================================================

rem Path to Unreal Editor (can be overridden via UE_ENGINE_ROOT env var)
if defined UE_ENGINE_ROOT (
    set UE_ROOT=%UE_ENGINE_ROOT%
) else (
    set UE_ROOT=d:\UE\UE_S
)

set UE_EDITOR_CMD="%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

rem Find .uproject file (4 levels up from scripts/)
set SEARCH_DIR=%~dp0..\..\..\..
for %%f in ("%SEARCH_DIR%\*.uproject") do (
    set UPROJECT_PATH="%%f"
    goto :found_project
)
echo [EAIS] ERROR: No .uproject file found
exit /b 1

:found_project
echo [EAIS] Running Automation Tests...
echo [EAIS] Project: %UPROJECT_PATH%

%UE_EDITOR_CMD% %UPROJECT_PATH% -ExecCmds="Automation RunTests EAIS; Quit" -unattended -nullrhi -testexit="Automation Test Queue Empty" -log -stdout -FullStdOutLogOutput

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Tests FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Tests PASSED!
endlocal
pause
