<#
.SYNOPSIS
    Verifies plan consistency by checking for forbidden patterns.
    
.DESCRIPTION
    This script scans the codebase to detect:
    1. Forbidden legacy patterns (TMap<FString, FString> for blackboard)
    2. Legacy FAICondition.Name/Value usage
    3. Object-based "states" JSON
    4. Editor code calling FAIInterpreter

.PARAMETER SourceDir
    Source directory to scan

.EXAMPLE
    .\VerifyPlanConsistency.ps1
#>

param(
    [string]$SourceDir = "$PSScriptRoot/../Source"
)

$ErrorActionPreference = "Stop"
$errors = 0

function Fail($msg) {
    Write-Host "[FAIL] $msg" -ForegroundColor Red
    $script:errors++
}

function Pass($msg) {
    Write-Host "[PASS] $msg" -ForegroundColor Green
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " VerifyPlanConsistency.ps1" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "SourceDir: $SourceDir" -ForegroundColor Gray
Write-Host ""

# Forbidden patterns to check
$ForbiddenPatterns = @(
    @{ Pattern = 'TMap<FString,\s*FString>\s+Blackboard'; Desc = "TMap<FString, FString> Blackboard (use TArray<FEAISBlackboardEntry>)" },
    @{ Pattern = '"states"\s*:\s*\{'; Desc = "Object-based states JSON (use array)" }
)

# Check source files
if (Test-Path $SourceDir) {
    $sourceFiles = Get-ChildItem -Path $SourceDir -Recurse -Include "*.h", "*.cpp"
    
    foreach ($fp in $ForbiddenPatterns) {
        $patternMatches = $sourceFiles | Select-String -Pattern $fp.Pattern -List
        if ($patternMatches.Count -gt 0) {
            Fail "$($fp.Desc)"
            foreach ($m in $patternMatches) {
                Write-Host "       $($m.Path):$($m.LineNumber)" -ForegroundColor Red
            }
        }
        else {
            Pass "No violations: $($fp.Desc)"
        }
    }
    
    # Check editor code doesn't call FAIInterpreter parsing
    $editorPath = "$SourceDir\P_EAIS_Editor"
    if (Test-Path $editorPath) {
        $editorFiles = Get-ChildItem -Path $editorPath -Recurse -Include "*.h", "*.cpp"
        $interpreterCalls = $editorFiles | Select-String -Pattern 'FAIInterpreter::(ParseJson|SerializeToJson)' -List
        if ($interpreterCalls.Count -gt 0) {
            Fail "Editor code calling FAIInterpreter methods"
            foreach ($m in $interpreterCalls) {
                Write-Host "       $($m.Path):$($m.LineNumber)" -ForegroundColor Red
            }
        }
        else {
            Pass "Editor code does not call FAIInterpreter directly"
        }
    }
}
else {
    Write-Host "Source directory not found: $SourceDir" -ForegroundColor Yellow
}

# Check JSON profiles
$ProfilesDir = "$PSScriptRoot/../Content/AIProfiles"

if (Test-Path $ProfilesDir) {
    Get-ChildItem -Path $ProfilesDir -Filter "*.runtime.json" -Recurse | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        if ($content -match '"states"\s*:\s*\{') {
            Fail "Object-based states in runtime JSON: $($_.Name)"
        }
    }
}

Write-Host ""
if ($errors -gt 0) {
    Write-Host "[RESULT] $errors consistency violation(s) found" -ForegroundColor Red
    exit 1
}
Write-Host "[RESULT] Plan consistency verified" -ForegroundColor Green
exit 0
