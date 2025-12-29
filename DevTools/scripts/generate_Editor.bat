@echo off
setlocal
rem ==============================================================================
rem EDITOR GENERATOR (VIA P_MWCS)
rem ==============================================================================

set UE_EDITOR_CMD="d:\UE\UE_S\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set UPROJECT_PATH="%~dp0..\..\..\..\A_MiniFootball.uproject"

echo [EAIS] Generating Editor Tool Widget...

%UE_EDITOR_CMD% %UPROJECT_PATH% -run=MWCS_CreateWidgets -Mode=ForceRecreate -SpecClasses="EAIS_EditorWidgetSpec" -unattended -nullrhi -nocpu -nosound -stdout -FullStdOutLogOutput

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Editor Generation FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Editor Generation SUCCESS!
endlocal
pause
