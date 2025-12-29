<#
.SYNOPSIS
    Automated P_EAIS Build, Test and Validation Script
    
.DESCRIPTION
    This script automates the full P_EAIS plugin build and validation pipeline:
    1. Builds the Unreal Engine project (includes P_EAIS plugin)
    2. Validates JSON AI Profiles against the schema
    3. Runs P_EAIS Automation Tests (headless) - optional with -RunTests
    4. Validates source code and plugin structure
    5. Displays comprehensive results and any errors/warnings
    
.PARAMETER SkipBuild
    Skip the project build step (useful if already built).
    
.PARAMETER TestOnly
    Only run automation tests, skip build and validation steps.

.PARAMETER ValidateOnly
    Only validate JSON profiles, skip build and tests.

.PARAMETER RunTests
    Run the Unreal automation tests (disabled by default as they can hang).

.PARAMETER TestTimeout
    Timeout in seconds for automation tests. Default: 120

.PARAMETER Profile
    Specific AI profile to test (e.g., "Striker", "Goalkeeper"). Defaults to all.

.PARAMETER VerboseOutput
    Show detailed output from all steps.

.EXAMPLE
    .\TestEAIS.ps1
    .\TestEAIS.ps1 -SkipBuild
    .\TestEAIS.ps1 -RunTests -TestTimeout 180
    .\TestEAIS.ps1 -ValidateOnly
    .\TestEAIS.ps1 -Profile "Striker" -VerboseOutput
#>

param(
    [switch]$SkipBuild,
    [switch]$TestOnly,
    [switch]$ValidateOnly,
    [switch]$RunTests,
    [int]$TestTimeout = 120,
    [string]$Profile = "",
    [switch]$VerboseOutput
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

$ProjectRoot = "D:\Projects\UE\A_MiniFootball"
$UEEngineRoot = "D:\UE\UE_S"
$UProjectPath = "$ProjectRoot\A_MiniFootball.uproject"

# P_EAIS paths
$PEAISRoot = "$ProjectRoot\Plugins\P_EAIS"
$PEAISSource = "$PEAISRoot\Source"
$AIProfilesDir = "$ProjectRoot\Content\AIProfiles"
$AISchemaPath = "$PEAISRoot\Docs\ai-schema.json"
$OutputDir = "$PEAISRoot\DevTools\output"

# UE paths
$UEEngineBuildBatch = "$UEEngineRoot\Engine\Build\BatchFiles\Build.bat"
$UEEditorCmd = "$UEEngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

# Create output directory if needed
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

function Write-ColoredText {
    param([string]$Text, [string]$Color = "White")
    Write-Host $Text -ForegroundColor $Color
}

function Write-Success {
    param([string]$Text)
    Write-Host "[OK] $Text" -ForegroundColor Green
}

function Write-ErrorMsg {
    param([string]$Text)
    Write-Host "[ERROR] $Text" -ForegroundColor Red
}

function Write-WarningMsg {
    param([string]$Text)
    Write-Host "[WARN] $Text" -ForegroundColor Yellow
}

function Write-Info {
    param([string]$Text)
    Write-Host "[INFO] $Text" -ForegroundColor White
}

function Write-Detail {
    param([string]$Text)
    if ($VerboseOutput) {
        Write-Host "       $Text" -ForegroundColor Gray
    }
}

function Show-LogAnalysis {
    param([string]$LogPath, [string]$FilterPattern = "EAIS|Error|Warning|Test|PASSED|FAILED")

    if (Test-Path $LogPath) {
        $Output = Get-Content $LogPath -Raw
        $RelevantLines = $Output -split "`n" | Where-Object { $_ -match $FilterPattern }
        
        if ($RelevantLines.Count -gt 0) {
            Write-Host ""
            Write-Host "--- Relevant Log Lines ---" -ForegroundColor Yellow
            $RelevantLines | Select-Object -First 50 | ForEach-Object {
                if ($_ -match "Error|FAILED") {
                    Write-Host $_ -ForegroundColor Red
                } elseif ($_ -match "Warning") {
                    Write-Host $_ -ForegroundColor Yellow
                } elseif ($_ -match "PASSED|Success|OK") {
                    Write-Host $_ -ForegroundColor Green
                } else {
                    Write-Host $_ -ForegroundColor White
                }
            }
            Write-Host "--- End Log ---" -ForegroundColor Yellow
        }
    }
}

function Test-JsonProfile {
    param([string]$JsonPath, [string]$SchemaPath)
    
    $Result = @{
        Path = $JsonPath
        Name = [System.IO.Path]::GetFileNameWithoutExtension($JsonPath)
        Valid = $false
        Errors = @()
        Warnings = @()
        StateCount = 0
        ActionCount = 0
    }
    
    try {
        $Content = Get-Content $JsonPath -Raw
        $Json = ConvertFrom-Json $Content
        
        if (-not $Json.name -and -not $Json.Name) {
            $Result.Errors += "Missing name field"
        } else {
            $Result.Name = if ($Json.name) { $Json.name } else { $Json.Name }
        }
        
        $States = $null
        if ($Json.states) { $States = $Json.states }
        elseif ($Json.States) { $States = $Json.States }
        
        if (-not $States) {
            $Result.Errors += "Missing states field"
        } else {
            if ($States -is [array]) {
                $Result.StateCount = $States.Count
            } else {
                $Result.StateCount = ($States | Get-Member -MemberType NoteProperty).Count
            }
            if ($Result.StateCount -eq 0) {
                $Result.Errors += "No states defined"
            }
        }
        
        $ActionMatches = [regex]::Matches($Content, '"Action"\s*:\s*"\w+"')
        $Result.ActionCount = $ActionMatches.Count
        
        $Result.Valid = ($Result.Errors.Count -eq 0)
    } catch {
        $Result.Errors += "JSON Parse Error: $($_.Exception.Message)"
    }
    
    return $Result
}

function Get-AIProfiles {
    param([string]$ProfileFilter = "")
    
    $Profiles = @()
    if (-not (Test-Path $AIProfilesDir)) { return $Profiles }
    
    Get-ChildItem -Path $AIProfilesDir -Filter "*.json" -Recurse | ForEach-Object {
        if ([string]::IsNullOrEmpty($ProfileFilter) -or $_.BaseName -like "*$ProfileFilter*") {
            $Profiles += $_.FullName
        }
    }
    return $Profiles
}

# ============================================================================
# Pre-flight Checks
# ============================================================================

Write-Header "P_EAIS Build and Validation Pipeline"
Write-Info "Project: $UProjectPath"
Write-Info "Plugin: $PEAISRoot"
Write-Info "Profiles: $AIProfilesDir"
Write-Host ""

if (-not (Test-Path $UProjectPath)) {
    Write-ErrorMsg "UProject file not found: $UProjectPath"
    exit 1
}

if (-not (Test-Path $PEAISRoot)) {
    Write-ErrorMsg "P_EAIS plugin not found: $PEAISRoot"
    exit 1
}

$TotalErrors = 0
$TotalWarnings = 0

# ============================================================================
# Step 1: Build Project
# ============================================================================

if (-not $TestOnly -and -not $ValidateOnly -and -not $SkipBuild) {
    Write-Header "Step 1: Building Project"
    
    if (-not (Test-Path $UEEngineBuildBatch)) {
        Write-ErrorMsg "UE Build.bat not found: $UEEngineBuildBatch"
        exit 1
    }

    Write-Info "Building A_MiniFootballEditor Win64 Development..."
    Write-Detail "This may take several minutes..."

    $BuildLogPath = "$OutputDir\build_output.txt"
    
    $BuildProcess = Start-Process -FilePath $UEEngineBuildBatch -ArgumentList @(
        "A_MiniFootballEditor", "Win64", "Development",
        "-Project=`"$UProjectPath`"", "-WaitMutex", "-FromMsBuild"
    ) -NoNewWindow -Wait -PassThru -RedirectStandardOutput $BuildLogPath

    if ($BuildProcess.ExitCode -eq 0) {
        Write-Success "Project built successfully"
    } else {
        Write-ErrorMsg "Project build failed! Exit code: $($BuildProcess.ExitCode)"
        Show-LogAnalysis -LogPath $BuildLogPath -FilterPattern "error|warning|failed"
        $TotalErrors++
        exit 1
    }
} elseif ($SkipBuild) {
    Write-Info "Skipping build step (-SkipBuild)"
} else {
    Write-Info "Skipping build step (TestOnly/ValidateOnly mode)"
}

# ============================================================================
# Step 2: Validate JSON AI Profiles
# ============================================================================

if (-not $TestOnly) {
    Write-Header "Step 2: Validating AI Profiles"
    
    $Profiles = Get-AIProfiles -ProfileFilter $Profile
    
    if ($Profiles.Count -eq 0) {
        Write-WarningMsg "No AI profiles found in $AIProfilesDir"
    } else {
        Write-Info "Found $($Profiles.Count) profile(s) to validate"
        Write-Host ""
        
        $ValidationResults = @()
        
        foreach ($ProfilePath in $Profiles) {
            $Result = Test-JsonProfile -JsonPath $ProfilePath -SchemaPath $AISchemaPath
            $ValidationResults += $Result
            
            $RelPath = $ProfilePath.Replace($AIProfilesDir, "").TrimStart("\")
            
            if ($Result.Valid) {
                Write-Host "  [PASS] " -ForegroundColor Green -NoNewline
                Write-Host "$RelPath" -ForegroundColor White -NoNewline
                Write-Host " (States: $($Result.StateCount), Actions: $($Result.ActionCount))" -ForegroundColor Gray
            } else {
                Write-Host "  [FAIL] " -ForegroundColor Red -NoNewline
                Write-Host "$RelPath" -ForegroundColor White
                foreach ($Err in $Result.Errors) {
                    Write-Host "         - $Err" -ForegroundColor Red
                }
                $TotalErrors++
            }
        }
        
        Write-Host ""
        $PassedCount = ($ValidationResults | Where-Object { $_.Valid }).Count
        Write-Info "Profile Validation: $PassedCount/$($Profiles.Count) passed"
    }
}

# ============================================================================
# Step 3: Run Automation Tests (optional - requires -RunTests)
# ============================================================================

if (-not $ValidateOnly -and $RunTests) {
    Write-Header "Step 3: Running Automation Tests"
    
    if (-not (Test-Path $UEEditorCmd)) {
        Write-ErrorMsg "UE Editor-Cmd not found: $UEEditorCmd"
        exit 1
    }

    Write-Info "Running EAIS.* automation tests (headless, timeout: ${TestTimeout}s)..."
    Write-Detail "This may take several minutes..."

    $TestLogPath = "$OutputDir\test_output.txt"
    $TestErrorPath = "$OutputDir\test_errors.txt"
    
    # Start the process without waiting
    $TestProcess = Start-Process -FilePath $UEEditorCmd -ArgumentList @(
        "`"$UProjectPath`"",
        "-ExecCmds=`"Automation RunTests EAIS; Quit`"",
        "-unattended", "-nullrhi", "-nop4", "-nosplash", "-nosound",
        "-stdout", "-FullStdOutLogOutput"
    ) -NoNewWindow -PassThru -RedirectStandardOutput $TestLogPath -RedirectStandardError $TestErrorPath

    # Wait with timeout
    $ProcessExited = $TestProcess.WaitForExit($TestTimeout * 1000)
    
    if (-not $ProcessExited) {
        Write-WarningMsg "Test process timed out after ${TestTimeout}s - killing process"
        $TestProcess.Kill()
        $TestProcess.WaitForExit(5000)
    }

    $TestOutput = ""
    if (Test-Path $TestLogPath) { $TestOutput = Get-Content $TestLogPath -Raw }
    
    $PassedTests = ([regex]::Matches($TestOutput, "Test Completed. Result=Passed")).Count
    $FailedTests = ([regex]::Matches($TestOutput, "Test Completed. Result=Failed")).Count
    $TotalTests = $PassedTests + $FailedTests
    
    Write-Host ""
    if ($TotalTests -gt 0) {
        Write-Info "Test Results: $PassedTests passed, $FailedTests failed (Total: $TotalTests)"
        if ($FailedTests -gt 0) {
            Write-ErrorMsg "$FailedTests test(s) failed!"
            $TotalErrors += $FailedTests
        } else {
            Write-Success "All tests passed!"
        }
    } else {
        Write-WarningMsg "No test results found. Tests may not have run properly."
        if ($VerboseOutput -and (Test-Path $TestLogPath)) {
            Write-Host ""
            Write-Host "--- Last 20 lines of test output ---" -ForegroundColor Yellow
            Get-Content $TestLogPath -Tail 20 | ForEach-Object { Write-Host $_ -ForegroundColor Gray }
        }
    }
} elseif (-not $ValidateOnly -and -not $RunTests) {
    Write-Header "Step 3: Automation Tests"
    Write-Info "Skipping automation tests (use -RunTests to enable)"
}

# ============================================================================
# Step 4: Source Code Analysis
# ============================================================================

Write-Header "Step 4: Source Code Analysis"

$SourceFiles = Get-ChildItem -Path $PEAISSource -Recurse -Include "*.h", "*.cpp"
$HeaderCount = ($SourceFiles | Where-Object { $_.Extension -eq ".h" }).Count
$CppCount = ($SourceFiles | Where-Object { $_.Extension -eq ".cpp" }).Count

Write-Info "Source Files: $HeaderCount headers, $CppCount implementations"

$TodoCount = 0
foreach ($File in $SourceFiles) {
    $Content = Get-Content $File.FullName -Raw
    $TodoCount += ([regex]::Matches($Content, "//\s*TODO")).Count
}

if ($TodoCount -gt 0) {
    Write-WarningMsg "Found $TodoCount TODO comments in source"
} else {
    Write-Success "No TODO comments found"
}

# ============================================================================
# Step 5: Plugin Structure Verification
# ============================================================================

Write-Header "Step 5: Plugin Structure Verification"

$RequiredFiles = @(
    "$PEAISRoot\P_EAIS.uplugin",
    "$PEAISRoot\README.md",
    "$PEAISRoot\GUIDE.md",
    "$PEAISRoot\Config\DefaultEAIS.ini",
    "$PEAISRoot\Docs\ai-schema.json",
    "$PEAISSource\P_EAIS\Public\PEAIS.h",
    "$PEAISSource\P_EAIS\Public\AIBehaviour.h",
    "$PEAISSource\P_EAIS\Public\AIInterpreter.h",
    "$PEAISSource\P_EAIS\Public\AIComponent.h",
    "$PEAISSource\P_EAIS\Public\AIAction.h",
    "$PEAISSource\P_EAIS\Public\EAISSubsystem.h",
    "$PEAISSource\P_EAIS\Public\EAIS_Types.h"
)

$MissingFiles = @()
foreach ($Required in $RequiredFiles) {
    if (Test-Path $Required) {
        Write-Detail "[EXISTS] $($Required.Replace($PEAISRoot, ''))"
    } else {
        $MissingFiles += $Required.Replace($PEAISRoot, "")
        $TotalErrors++
    }
}

if ($MissingFiles.Count -eq 0) {
    Write-Success "All required files present ($($RequiredFiles.Count) files)"
} else {
    Write-ErrorMsg "Missing $($MissingFiles.Count) required file(s):"
    foreach ($Missing in $MissingFiles) {
        Write-Host "  - $Missing" -ForegroundColor Red
    }
}

# ============================================================================
# Step 6: DevTools Verification
# ============================================================================

Write-Header "Step 6: DevTools Verification"

$DevToolsFiles = @(
    "$PEAISRoot\DevTools\scripts\build_headless.bat",
    "$PEAISRoot\DevTools\scripts\build_headless.sh",
    "$PEAISRoot\DevTools\scripts\run_tests.bat",
    "$PEAISRoot\DevTools\scripts\run_tests.sh",
    "$PEAISRoot\DevTools\scripts\generate_Editor.bat",
    "$PEAISRoot\DevTools\ci\eais_ci.yml"
)

$DevToolsMissing = @()
foreach ($DevTool in $DevToolsFiles) {
    if (Test-Path $DevTool) {
        Write-Detail "[EXISTS] $([System.IO.Path]::GetFileName($DevTool))"
    } else {
        $DevToolsMissing += [System.IO.Path]::GetFileName($DevTool)
        $TotalWarnings++
    }
}

if ($DevToolsMissing.Count -eq 0) {
    Write-Success "All DevTools present ($($DevToolsFiles.Count) files)"
} else {
    Write-WarningMsg "Missing $($DevToolsMissing.Count) DevTools file(s):"
    foreach ($Missing in $DevToolsMissing) {
        Write-Host "  - $Missing" -ForegroundColor Yellow
    }
}

# ============================================================================
# Summary
# ============================================================================

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Pipeline Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Plugin: P_EAIS" -ForegroundColor White
Write-Host "  Location: $PEAISRoot" -ForegroundColor Gray
Write-Host ""

if ($TotalErrors -eq 0 -and $TotalWarnings -eq 0) {
    Write-Host "  [PASS] All checks PASSED!" -ForegroundColor Green
} elseif ($TotalErrors -eq 0) {
    Write-Host "  [WARN] Passed with $TotalWarnings warning(s)" -ForegroundColor Yellow
} else {
    Write-Host "  [FAIL] $TotalErrors error(s), $TotalWarnings warning(s)" -ForegroundColor Red
}

Write-Host ""
Write-Host "  Logs: $OutputDir" -ForegroundColor Gray
Write-Host ""

# ============================================================================
# Quick Reference
# ============================================================================

if ($VerboseOutput) {
    Write-Header "Quick Reference"
    Write-Host ""
    Write-Host "  Console Commands:" -ForegroundColor White
    Write-Host "    EAIS.SpawnBot Team Profile  - Spawn AI bot" -ForegroundColor Gray
    Write-Host "    EAIS.Debug 0 or 1           - Toggle debug mode" -ForegroundColor Gray
    Write-Host "    EAIS.InjectEvent AI Event   - Inject event" -ForegroundColor Gray
    Write-Host "    EAIS.ListActions            - List registered actions" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  AI Profiles:" -ForegroundColor White
    $AllProfiles = Get-AIProfiles
    foreach ($ProfPath in $AllProfiles) {
        $ProfName = [System.IO.Path]::GetFileNameWithoutExtension($ProfPath)
        Write-Host "    - $ProfName" -ForegroundColor Gray
    }
    Write-Host ""
}

# Exit with appropriate code
if ($TotalErrors -gt 0) {
    exit 1
} else {
    exit 0
}
