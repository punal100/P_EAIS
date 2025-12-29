@echo off
setlocal
rem ==============================================================================
rem HEADLESS TEST RUNNER
rem ==============================================================================

set UE_EDITOR_CMD="d:\UE\UE_S\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set UPROJECT_PATH="%~dp0..\..\..\..\A_MiniFootball.uproject"

echo [EAIS] Running Automation Tests...

%UE_EDITOR_CMD% %UPROJECT_PATH% -ExecCmds="Automation RunTests EAIS; Quit" -unattended -nullrhi -testexit="Automation Test Queue Empty" -log -stdout -FullStdOutLogOutput

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Tests FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Tests PASSED!
endlocal
pause
