@echo off
setlocal
rem ==============================================================================
rem HEADLESS BUILD SCRIPT
rem ==============================================================================

rem Path to Unreal Editor Commandlet
set UE_EDITOR_CMD="d:\UE\UE_S\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

rem Path to Project file (4 levels up from scripts/)
set UPROJECT_PATH="%~dp0..\..\..\..\A_MiniFootball.uproject"

echo [EAIS] Building Plugin...
rem Running a build command requires RunUAT usually, but for a plugin we can just ensure it compiles by running editor or commandlet?
rem Actually, proper plugin build uses RunUAT BuildPlugin. 
rem But for "Headless Build" in the context of a project, usually we compile the project.
rem Let's use UnrealBuildTool via RunUAT for a proper plugin build if possible, or just build the project.
rem Simplest check: Build the project.

set RUNUAT_CMD="d:\UE\UE_S\Engine\Build\BatchFiles\RunUAT.bat"

echo [EAIS] Building Project (Includes Plugin)...
call %RUNUAT_CMD% BuildCookRun -project=%UPROJECT_PATH% -noP4 -clientconfig=Development -serverconfig=Development -platform=Win64 -build -unattended -utf8output

if %ERRORLEVEL% NEQ 0 (
    echo [EAIS] Build FAILED!
    exit /b %ERRORLEVEL%
)

echo [EAIS] Build SUCCESS!
endlocal
pause
