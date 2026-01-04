@echo off
setlocal
rem ==============================================================================
rem EDITOR GENERATOR (VIA P_MWCS)
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
echo [EAIS] Generating Editor Tool Widget...
echo [EAIS] Project: %UPROJECT_PATH%

%UE_EDITOR_CMD% %UPROJECT_PATH% -run=MWCS_CreateWidgets -Mode=ForceRecreate -unattended -nullrhi -nocpu -nosound -stdout -FullStdOutLogOutput

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Editor Generation FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Editor Generation SUCCESS!
endlocal
pause
