<#
.SYNOPSIS
Master EAIS test runner - builds, tests, and validates P_EAIS plugin.

.DESCRIPTION
Orchestrates all P_EAIS tests including:
- Build validation
- JSON profile validation
- Automation tests
- Source code analysis

Auto-detects project and UE installation.

.PARAMETER ProjectPath
(Optional) Path to project root. If not provided, auto-detect.

.PARAMETER UEPath
(Optional) Path to UE installation. Default: D:\UE\UE_S

.PARAMETER SkipBuild
Skip the build step (faster for iteration)

.PARAMETER RunTests
Run UE automation tests

.PARAMETER Verbose
Enable verbose output

.EXAMPLE
# Full validation
.\RunEAISTests.ps1

# Skip build, run tests
.\RunEAISTests.ps1 -SkipBuild -RunTests

# Specify paths
.\RunEAISTests.ps1 -ProjectPath "D:\MyGame" -UEPath "D:\UE\UE_5.5"
#>

param(
    [string]$ProjectPath = $null,
    [string]$UEPath = $null,
    [switch]$SkipBuild,
    [switch]$RunTests,
    [switch]$RegenerateEUW,
    [int]$Timeout = 300,
    [switch]$VerboseOutput
)

$ErrorActionPreference = 'Stop'

# ============================================
# INITIALIZATION
# ============================================

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "     P_EAIS Automated Testing Framework v1.0                    " -ForegroundColor Cyan
Write-Host "     (Enhanced AI System - Full Test Suite)                     " -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# Get script and plugin directories
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptDir "..\..") ).Path
$DevToolsRoot = (Resolve-Path (Join-Path $ScriptDir "..") ).Path

# ============================================
# HELPER FUNCTIONS
# ============================================

function Write-Log {
    param([string]$Message, [string]$Level = "Info")
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    switch ($Level) {
        "Success" { Write-Host "[$timestamp] [OK] $Message" -ForegroundColor Green }
        "Error" { Write-Host "[$timestamp] [ERROR] $Message" -ForegroundColor Red }
        "Warning" { Write-Host "[$timestamp] [WARN] $Message" -ForegroundColor Yellow }
        default { Write-Host "[$timestamp] $Message" -ForegroundColor Gray }
    }
}

function Find-ProjectFile {
    param([string]$StartPath)
    
    $searchPath = $StartPath
    for ($i = 0; $i -lt 5; $i++) {
        $uprojects = Get-ChildItem -Path $searchPath -Filter "*.uproject" -File -ErrorAction SilentlyContinue
        if ($uprojects) {
            return $uprojects[0]
        }
        $searchPath = Split-Path -Parent $searchPath
    }
    return $null
}

# ============================================
# PROJECT & UE DETECTION
# ============================================

Write-Log "Detecting environment..."

# Detect project
if (-not $ProjectPath) {
    $ProjectFile = Find-ProjectFile -StartPath $PluginRoot
    if (-not $ProjectFile) {
        Write-Log "Could not detect project. Specify -ProjectPath" -Level Error
        exit 1
    }
    $ProjectPath = Split-Path -Parent $ProjectFile.FullName
}
else {
    $uprojects = Get-ChildItem -Path $ProjectPath -Filter "*.uproject" -File -ErrorAction SilentlyContinue
    if (-not $uprojects) {
        Write-Log "No .uproject found in $ProjectPath" -Level Error
        exit 1
    }
    $ProjectFile = $uprojects[0]
}

$ProjectName = [System.IO.Path]::GetFileNameWithoutExtension($ProjectFile.Name)

# Detect UE
if (-not $UEPath) {
    $UEPath = if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { "D:\UE\UE_S" }
}

$UEEditorCmd = Join-Path $UEPath "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$UEBuildBat = Join-Path $UEPath "Engine\Build\BatchFiles\Build.bat"

Write-Log "Project: $($ProjectFile.FullName)" -Level Success
Write-Log "UE Path: $UEPath" -Level Success
Write-Log "Plugin:  $PluginRoot" -Level Success
Write-Host ""

# ============================================
# TEST EXECUTION
# ============================================

$allPassed = $true
$testResults = @()

# ----------------------------------------
# Step 1: Build
# ----------------------------------------
if (-not $SkipBuild) {
    Write-Log "════════════════════════════════════════════════════════════════"
    Write-Log "Step 1: Building Project"
    Write-Log "════════════════════════════════════════════════════════════════"
    
    if (-not (Test-Path $UEBuildBat)) {
        Write-Log "Build.bat not found at $UEBuildBat" -Level Error
        $testResults += "Build: FAIL"
        $allPassed = $false
    }
    else {
        $buildArgs = @(
            "${ProjectName}Editor", "Win64", "Development",
            "-Project=`"$($ProjectFile.FullName)`"", "-WaitMutex", "-FromMsBuild"
        )
        
        Write-Log "Building ${ProjectName}Editor..."
        $buildProcess = Start-Process -FilePath $UEBuildBat -ArgumentList $buildArgs -NoNewWindow -Wait -PassThru
        
        if ($buildProcess.ExitCode -eq 0) {
            Write-Log "Build PASSED" -Level Success
            $testResults += "Build: PASS"
        }
        else {
            Write-Log "Build FAILED (exit code: $($buildProcess.ExitCode))" -Level Error
            $testResults += "Build: FAIL"
            $allPassed = $false
        }
    }
    Write-Host ""
}
else {
    Write-Log "Skipping build step (-SkipBuild)" -Level Warning
    $testResults += "Build: SKIP"
    Write-Host ""
}

# ----------------------------------------
# Step 2: JSON Validation
# ----------------------------------------
Write-Log "════════════════════════════════════════════════════════════════"
Write-Log "Step 2: Validating AI Profiles"
Write-Log "════════════════════════════════════════════════════════════════"

$validateScript = Join-Path $ScriptDir "ValidateAIJson.ps1"
if (Test-Path $validateScript) {
    $profilesDir = Join-Path $PluginRoot "Content\AIProfiles"
    $editorDir = Join-Path $PluginRoot "Editor\AI"
    $validateResult = & $validateScript -ProfilesDir $profilesDir -EditorDir $editorDir
    if ($LASTEXITCODE -eq 0) {
        Write-Log "JSON Validation PASSED" -Level Success
        $testResults += "JSONValidation: PASS"
    }
    else {
        Write-Log "JSON Validation FAILED" -Level Error
        $testResults += "JSONValidation: FAIL"
        $allPassed = $false
    }
}
else {
    Write-Log "ValidateAIJson.ps1 not found" -Level Warning
    $testResults += "JSONValidation: SKIP"
}
Write-Host ""

# ----------------------------------------
# Step 3: Plan Consistency
# ----------------------------------------
Write-Log "════════════════════════════════════════════════════════════════"
Write-Log "Step 3: Verifying Plan Consistency"
Write-Log "════════════════════════════════════════════════════════════════"

$consistencyScript = Join-Path $ScriptDir "VerifyPlanConsistency.ps1"
if (Test-Path $consistencyScript) {
    $consistencyResult = & $consistencyScript
    if ($LASTEXITCODE -eq 0) {
        Write-Log "Plan Consistency PASSED" -Level Success
        $testResults += "PlanConsistency: PASS"
    }
    else {
        Write-Log "Plan Consistency FAILED" -Level Error
        $testResults += "PlanConsistency: FAIL"
        $allPassed = $false
    }
}
else {
    Write-Log "VerifyPlanConsistency.ps1 not found" -Level Warning
    $testResults += "PlanConsistency: SKIP"
}
Write-Host ""

# ----------------------------------------
# Step 4: Automation Tests
# ----------------------------------------
if ($RunTests) {
    Write-Log "════════════════════════════════════════════════════════════════"
    Write-Log "Step 4: Running Automation Tests"
    Write-Log "════════════════════════════════════════════════════════════════"
    
    if (-not (Test-Path $UEEditorCmd)) {
        Write-Log "UnrealEditor-Cmd.exe not found" -Level Error
        $testResults += "AutomationTests: FAIL"
        $allPassed = $false
    }
    else {
        Write-Log "Running EAIS automation tests..."
        $testArgs = @(
            "`"$($ProjectFile.FullName)`"",
            "-Map=/Engine/Maps/Entry",
            "-ExecCmds=`"Automation RunTests EAIS; Quit`"",
            "-unattended", "-nullrhi", "-nop4",
            "-testexit=`"Automation Test Queue Empty`"",
            "-stdout", "-FullStdOutLogOutput"
        )

        $outputDir = Join-Path $DevToolsRoot "output"
        if (-not (Test-Path $outputDir)) {
            New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
        }
        $testStdout = Join-Path $outputDir "eais_tests_stdout.log"
        $testStderr = Join-Path $outputDir "eais_tests_stderr.log"

        $testProcess = Start-Process -FilePath $UEEditorCmd -ArgumentList $testArgs -NoNewWindow -PassThru -RedirectStandardOutput $testStdout -RedirectStandardError $testStderr
        $waited = $testProcess | Wait-Process -Timeout $Timeout -ErrorAction SilentlyContinue
        if (-not $waited) {
            Write-Log "Automation tests timed out after ${Timeout}s; terminating UnrealEditor-Cmd" -Level Error
            Stop-Process -Id $testProcess.Id -Force -ErrorAction SilentlyContinue
            $testResults += "AutomationTests: FAIL"
            $allPassed = $false
        }
        elseif ($testProcess.ExitCode -eq 0) {
            Write-Log "Automation Tests PASSED" -Level Success
            $testResults += "AutomationTests: PASS"
        }
        else {
            Write-Log "Automation Tests FAILED" -Level Error
            $testResults += "AutomationTests: FAIL"
            $allPassed = $false
        }
    }
    Write-Host ""
}
else {
    Write-Log "Skipping automation tests (use -RunTests to enable)" -Level Warning
    $testResults += "AutomationTests: SKIP"
    Write-Host ""
}

# ----------------------------------------
# ----------------------------------------
# Step 5: Regenerate EUW (optional)
# ----------------------------------------
if ($RegenerateEUW) {
    Write-Log "════════════════════════════════════════════════════════════════"
    Write-Log "Step 5: Regenerating EUW"
    Write-Log "════════════════════════════════════════════════════════════════"

    if (-not (Test-Path $UEEditorCmd)) {
        Write-Log "UnrealEditor-Cmd.exe not found" -Level Error
        $testResults += "EUWRegenerate: FAIL"
        $allPassed = $false
    }
    else {
        $euwArgs = @(
            "`"$($ProjectFile.FullName)`"",
            "-run=EAIS_GenerateEUW",
            "-unattended", "-nop4", "-NullRHI",
            "-stdout", "-FullStdOutLogOutput"
        )

        $outputDir = Join-Path $DevToolsRoot "output"
        if (-not (Test-Path $outputDir)) {
            New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
        }
        $euwStdout = Join-Path $outputDir "eais_generate_euw_stdout.log"
        $euwStderr = Join-Path $outputDir "eais_generate_euw_stderr.log"

        Write-Log "Running EAIS_GenerateEUW commandlet..."
        $euwProcess = Start-Process -FilePath $UEEditorCmd -ArgumentList $euwArgs -NoNewWindow -PassThru -RedirectStandardOutput $euwStdout -RedirectStandardError $euwStderr
        $waited = $euwProcess | Wait-Process -Timeout $Timeout -ErrorAction SilentlyContinue
        if (-not $waited) {
            Write-Log "EUW regeneration timed out after ${Timeout}s; terminating UnrealEditor-Cmd" -Level Error
            Stop-Process -Id $euwProcess.Id -Force -ErrorAction SilentlyContinue
            $testResults += "EUWRegenerate: FAIL"
            $allPassed = $false
        }
        elseif ($euwProcess.ExitCode -eq 0) {
            Write-Log "EUW regeneration PASSED" -Level Success
            $testResults += "EUWRegenerate: PASS"
        }
        else {
            Write-Log "EUW regeneration FAILED" -Level Error
            $testResults += "EUWRegenerate: FAIL"
            $allPassed = $false
        }
    }
    Write-Host ""
}
else {
    Write-Log "Skipping EUW regeneration (use -RegenerateEUW to enable)" -Level Warning
    $testResults += "EUWRegenerate: SKIP"
    Write-Host ""
}

# ----------------------------------------
# Step 6: Source Analysis
# ----------------------------------------
Write-Log "════════════════════════════════════════════════════════════════"
Write-Log "Step 6: Source Code Analysis"
Write-Log "════════════════════════════════════════════════════════════════"

$sourceDir = Join-Path $PluginRoot "Source"
$headerCount = (Get-ChildItem -Path $sourceDir -Recurse -Filter "*.h" -File).Count
$cppCount = (Get-ChildItem -Path $sourceDir -Recurse -Filter "*.cpp" -File).Count
$todoCount = (Get-ChildItem -Path $sourceDir -Recurse -Include "*.h", "*.cpp" | Select-String -Pattern "TODO" -List).Count

Write-Log "Source files: $headerCount headers, $cppCount implementations"
if ($todoCount -gt 0) {
    Write-Log "Found $todoCount TODO comments" -Level Warning
}
$testResults += "SourceAnalysis: PASS"
Write-Host ""

# ----------------------------------------
# Step 7: Plugin Structure
# ----------------------------------------
Write-Log "════════════════════════════════════════════════════════════════"
Write-Log "Step 7: Plugin Structure Verification"
Write-Log "════════════════════════════════════════════════════════════════"

$requiredFiles = @(
    "P_EAIS.uplugin",
    "README.md",
    "GUIDE.md",
    "Source\P_EAIS\Public\EAIS_Types.h",
    "Source\P_EAIS\Public\AIInterpreter.h",
    "Source\P_EAIS\Public\AIComponent.h",
    "Source\P_EAIS_Editor\Public\SEAIS_GraphEditor.h",
    "DevTools\scripts\ValidateAIJson.ps1"
)

$missingFiles = @()
foreach ($file in $requiredFiles) {
    $fullPath = Join-Path $PluginRoot $file
    if (Test-Path $fullPath) {
        if ($VerboseOutput) {
            Write-Log "[EXISTS] $file"
        }
    }
    else {
        Write-Log "[MISSING] $file" -Level Error
        $missingFiles += $file
    }
}

if ($missingFiles.Count -eq 0) {
    Write-Log "All required files present ($($requiredFiles.Count) files)" -Level Success
    $testResults += "PluginStructure: PASS"
}
else {
    Write-Log "Missing $($missingFiles.Count) required files" -Level Error
    $testResults += "PluginStructure: FAIL"
    $allPassed = $false
}
Write-Host ""

# ============================================
# SUMMARY
# ============================================

Write-Log "════════════════════════════════════════════════════════════════"
Write-Log "TEST SUMMARY"
Write-Log "════════════════════════════════════════════════════════════════"

foreach ($result in $testResults) {
    if ($result -like "*PASS*") {
        Write-Log $result -Level Success
    }
    elseif ($result -like "*SKIP*") {
        Write-Log $result -Level Warning
    }
    else {
        Write-Log $result -Level Error
    }
}

Write-Log "════════════════════════════════════════════════════════════════"

if ($allPassed) {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Green
    Write-Host "          ALL TESTS PASSED                                     " -ForegroundColor Green
    Write-Host "================================================================" -ForegroundColor Green
    Write-Host ""
    exit 0
}
else {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Red
    Write-Host "          SOME TESTS FAILED                                    " -ForegroundColor Red
    Write-Host "================================================================" -ForegroundColor Red
    Write-Host ""
    exit 1
}
