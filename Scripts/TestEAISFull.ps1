<#
.SYNOPSIS
    Automated P_EAIS Build, Test, and EUW Generation Script
    
.DESCRIPTION
    This script automates the full P_EAIS development pipeline:
    1. Builds the UE project (includes P_EAIS plugin)
    2. Validates AI JSON profiles
    3. Verifies plan consistency (no forbidden patterns)
    4. Regenerates EUW via P_MWCS
    5. Runs automation tests (optional)
    6. Displays results
    
.PARAMETER SkipBuild
    Skip the project build step (faster for iteration).
    
.PARAMETER ValidateOnly
    Only run validation, skip build and regeneration.
    
.PARAMETER RunTests
    Run UE automation tests after validation.
    
.PARAMETER Regenerate
    Force regenerate the EUW using P_MWCS.

.EXAMPLE
    .\TestEAISFull.ps1                    # Full pipeline
    .\TestEAISFull.ps1 -SkipBuild         # Skip build
    .\TestEAISFull.ps1 -ValidateOnly      # Only validation
    .\TestEAISFull.ps1 -Regenerate        # Regenerate EUW
    .\TestEAISFull.ps1 -RunTests          # Include automation tests
#>

param(
    [switch]$SkipBuild,
    [switch]$ValidateOnly,
    [switch]$RunTests,
    [switch]$Regenerate
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration (Auto-detect paths)
# ============================================================================

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PEAISRoot = Split-Path -Parent $ScriptDir
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PEAISRoot)

# Find .uproject file
$UProjectFile = Get-ChildItem -Path $ProjectRoot -Filter "*.uproject" -File | Select-Object -First 1
if (-not $UProjectFile) {
    Write-Host "[ERROR] No .uproject file found in $ProjectRoot" -ForegroundColor Red
    exit 1
}
$UProjectPath = $UProjectFile.FullName
$ProjectName = [System.IO.Path]::GetFileNameWithoutExtension($UProjectFile.Name)

# UE Engine path
$UEEngineRoot = if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { "D:\UE\UE_S" }
$UEEngineBuildBatch = "$UEEngineRoot\Engine\Build\BatchFiles\Build.bat"
$UEEditorCmd = "$UEEngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

# Output directory
$OutputDir = "$PEAISRoot\DevTools\output"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

# ============================================================================
# Functions
# ============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host " $Text" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Text)
    Write-Host "[OK] $Text" -ForegroundColor Green
}

function Write-ErrorMsg {
    param([string]$Text)
    Write-Host "[ERROR] $Text" -ForegroundColor Red
}

function Write-WarnMsg {
    param([string]$Text)
    Write-Host "[WARN] $Text" -ForegroundColor Yellow
}

function Write-Info {
    param([string]$Text)
    Write-Host "[INFO] $Text" -ForegroundColor White
}

function Show-LogAnalysis {
    param([string]$LogPath)

    if (Test-Path $LogPath) {
        $Output = Get-Content $LogPath -Raw
        
        $RelevantLines = $Output -split "`n" | Where-Object {
            $_ -match "MWCS" -or
            $_ -match "EAIS" -or
            $_ -match "Error" -or
            $_ -match "Warning" -or
            $_ -match "Created" -or
            $_ -match "Failed"
        }
        
        if ($RelevantLines.Count -gt 0) {
            Write-Host ""
            Write-Host "--- Relevant Log Lines ---" -ForegroundColor Yellow
            foreach ($line in $RelevantLines | Select-Object -First 20) {
                if ($line -match "Error") {
                    Write-Host $line -ForegroundColor Red
                }
                elseif ($line -match "Warning") {
                    Write-Host $line -ForegroundColor Yellow
                }
                else {
                    Write-Host $line -ForegroundColor White
                }
            }
            Write-Host "--- End Log ---" -ForegroundColor Yellow
        }
    }
}

# ============================================================================
# Start Pipeline
# ============================================================================

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  P_EAIS Build, Test & Validation      " -ForegroundColor Cyan
Write-Host "  Automated Pipeline v1.0              " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Info "Project: $UProjectPath"
Write-Info "Plugin:  $PEAISRoot"
Write-Info "Engine:  $UEEngineRoot"
Write-Host ""

$AllPassed = $true

# ============================================================================
# Step 1: Build Project
# ============================================================================

if (-not $ValidateOnly -and -not $SkipBuild) {
    Write-Header "Step 1: Building Project"
    
    if (-not (Test-Path $UEEngineBuildBatch)) {
        Write-ErrorMsg "UE Build.bat not found: $UEEngineBuildBatch"
        exit 1
    }

    Write-Info "Building ${ProjectName}Editor Win64 Development..."
    Write-Info "This may take several minutes..."

    $BuildArgs = @(
        "${ProjectName}Editor",
        "Win64",
        "Development",
        "-Project=`"$UProjectPath`"",
        "-WaitMutex",
        "-FromMsBuild"
    )

    $BuildProcess = Start-Process -FilePath $UEEngineBuildBatch `
        -ArgumentList $BuildArgs `
        -NoNewWindow `
        -Wait `
        -PassThru

    if ($BuildProcess.ExitCode -eq 0) {
        Write-Success "Project built successfully"
    }
    else {
        Write-ErrorMsg "Build failed! Exit code: $($BuildProcess.ExitCode)"
        exit 1
    }
}
elseif ($SkipBuild) {
    Write-Info "Skipping build step (-SkipBuild)"
}

# ============================================================================
# Step 2: Validate AI Profiles
# ============================================================================

Write-Header "Step 2: Validating AI Profiles"

$ValidateScript = Join-Path $ScriptDir "ValidateAIJson.ps1"
if (Test-Path $ValidateScript) {
    & $ValidateScript
    if ($LASTEXITCODE -eq 0) {
        Write-Success "AI Profile validation passed"
    }
    else {
        Write-ErrorMsg "AI Profile validation failed!"
        $AllPassed = $false
    }
}
else {
    Write-WarnMsg "ValidateAIJson.ps1 not found, skipping"
}

# ============================================================================
# Step 3: Verify Plan Consistency
# ============================================================================

Write-Header "Step 3: Verifying Plan Consistency"

$ConsistencyScript = Join-Path $ScriptDir "VerifyPlanConsistency.ps1"
if (Test-Path $ConsistencyScript) {
    & $ConsistencyScript
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Plan consistency verified"
    }
    else {
        Write-ErrorMsg "Plan consistency check failed!"
        $AllPassed = $false
    }
}
else {
    Write-WarnMsg "VerifyPlanConsistency.ps1 not found, skipping"
}

# ============================================================================
# Step 4: Regenerate EUW via P_MWCS
# ============================================================================

if ($Regenerate -or (-not $ValidateOnly)) {
    Write-Header "Step 4: Regenerating EUW via P_MWCS"
    
    if (-not (Test-Path $UEEditorCmd)) {
        Write-ErrorMsg "UE Editor-Cmd not found: $UEEditorCmd"
        exit 1
    }

    # Delete old EUW if exists
    $OldEuw = "$ProjectRoot\Content\Editor\EAIS\EUW_EAIS_AIEditor.uasset"
    if (Test-Path $OldEuw) {
        Remove-Item $OldEuw -Force -ErrorAction SilentlyContinue
        Write-Info "Deleted old EUW asset"
    }

    Write-Info "Running EAIS_GenerateEUW commandlet..."

    $CommonArgs = @(
        "`"$UProjectPath`"",
        "-run=EAIS_GenerateEUW",
        "-unattended",
        "-nop4",
        "-NullRHI",
        "-stdout",
        "-FullStdOutLogOutput"
    )

    $CreateLogPath = "$OutputDir\eais_generate_output.txt"
    
    $CreateProcess = Start-Process -FilePath $UEEditorCmd `
        -ArgumentList $CommonArgs `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $CreateLogPath `
        -RedirectStandardError "$OutputDir\eais_generate_errors.txt"
        
    Show-LogAnalysis -LogPath $CreateLogPath

    if ($CreateProcess.ExitCode -eq 0) {
        Write-Success "EUW regenerated successfully"
    }
    else {
        Write-WarnMsg "EUW regeneration had issues (exit: $($CreateProcess.ExitCode))"
    }
}
elseif ($ValidateOnly) {
    Write-Info "Skipping EUW regeneration (-ValidateOnly)"
}

# ============================================================================
# Step 5: Run Automation Tests
# ============================================================================

if ($RunTests) {
    Write-Header "Step 5: Running Automation Tests"
    
    if (-not (Test-Path $UEEditorCmd)) {
        Write-ErrorMsg "UE Editor-Cmd not found"
        $AllPassed = $false
    }
    else {
        Write-Info "Running EAIS automation tests..."

        $TestArgs = @(
            "`"$UProjectPath`"",
            "-ExecCmds=`"Automation RunTests EAIS; Quit`"",
            "-unattended",
            "-nullrhi",
            "-nop4",
            "-testexit=`"Automation Test Queue Empty`"",
            "-stdout",
            "-FullStdOutLogOutput"
        )
        
        $TestLogPath = "$OutputDir\test_output.txt"
        
        $TestProcess = Start-Process -FilePath $UEEditorCmd `
            -ArgumentList $TestArgs `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $TestLogPath `
            -RedirectStandardError "$OutputDir\test_errors.txt"
            
        Show-LogAnalysis -LogPath $TestLogPath

        if ($TestProcess.ExitCode -eq 0) {
            Write-Success "Automation tests passed"
        }
        else {
            Write-ErrorMsg "Automation tests failed! Exit: $($TestProcess.ExitCode)"
            $AllPassed = $false
        }
    }
}
else {
    Write-Info "Skipping automation tests (use -RunTests to enable)"
}

# ============================================================================
# Step 6: Verify Plugin Structure
# ============================================================================

Write-Header "Step 6: Verifying Plugin Structure"

$RequiredFiles = @(
    "P_EAIS.uplugin",
    "README.md",
    "GUIDE.md",
    "Source\P_EAIS\Public\EAIS_Types.h",
    "Source\P_EAIS\Public\AIComponent.h",
    "Source\P_EAIS\Public\AIInterpreter.h",
    "Source\P_EAIS_Editor\Public\SEAIS_GraphEditor.h",
    "Source\P_EAISTools\Public\EAIS_AIEditor.h",
    "Scripts\ValidateAIJson.ps1"
)

$MissingCount = 0
foreach ($file in $RequiredFiles) {
    $fullPath = Join-Path $PEAISRoot $file
    if (-not (Test-Path $fullPath)) {
        Write-ErrorMsg "Missing: $file"
        $MissingCount++
    }
}

if ($MissingCount -eq 0) {
    Write-Success "All required files present ($($RequiredFiles.Count) files)"
}
else {
    Write-ErrorMsg "$MissingCount required files missing!"
    $AllPassed = $false
}

# ============================================================================
# Summary
# ============================================================================

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Pipeline Complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Plugin: $PEAISRoot" -ForegroundColor White
Write-Host "Logs:   $OutputDir" -ForegroundColor White
Write-Host ""

if ($AllPassed) {
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  ALL CHECKS PASSED" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    exit 0
}
else {
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  SOME CHECKS FAILED" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    exit 1
}
