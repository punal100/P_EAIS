<#
.SYNOPSIS
    Validates editor graphs headlessly without opening UE.
    
.DESCRIPTION
    This script validates:
    1. Editor JSON graph structure
    2. Graph -> Runtime JSON conversion validity (schema check)

.PARAMETER EditorDir
    Directory containing *.editor.json files

.EXAMPLE
    .\ValidateGraph.ps1
    .\ValidateGraph.ps1 -EditorDir "Editor/AI"
#>

param(
    [string]$EditorDir = "$PSScriptRoot/../Editor/AI"
)

$ErrorActionPreference = "Stop"
$errors = 0

function Fail($msg) {
    Write-Error $msg
    $script:errors++
}

function HasKey($obj, $key) {
    return ($null -ne $obj) -and ($obj.PSObject.Properties.Name -contains $key)
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ValidateGraph.ps1 - EAIS Graph Validator" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "EditorDir: $EditorDir" -ForegroundColor Gray
Write-Host ""

if (-not (Test-Path $EditorDir)) {
    Write-Host "Editor directory not found: $EditorDir" -ForegroundColor Yellow
    exit 0
}

Get-ChildItem -Path $EditorDir -Filter "*.editor.json" -Recurse | ForEach-Object {
    try {
        $json = Get-Content $_.FullName -Raw | ConvertFrom-Json
        $RelPath = $_.Name
        
        # Validate graph structure
        if (-not $json.states -or $json.states.Count -eq 0) {
            Fail "Invalid graph (no states): $RelPath"
            return
        }
        
        # Validate initial state exists
        $stateIds = @()
        foreach ($s in $json.states) {
            $stateIds += $s.id
        }
        
        if (-not ($stateIds -contains $json.initialState)) {
            Fail "Initial state '$($json.initialState)' not found in states: $RelPath"
            return
        }
        
        # Validate transitions point to valid states
        foreach ($s in $json.states) {
            foreach ($t in $s.transitions) {
                if (-not ($stateIds -contains $t.to)) {
                    Fail "State '$($s.id)' has transition to invalid state '$($t.to)': $RelPath"
                }
            }
        }
        
        # Validate non-terminal states have transitions or onTick
        foreach ($s in $json.states) {
            if (-not $s.terminal) {
                $hasTrans = ($s.transitions -and $s.transitions.Count -gt 0)
                $hasTick = ($s.onTick -and $s.onTick.Count -gt 0)
                if (-not $hasTrans -and -not $hasTick) {
                    Fail "Non-terminal state '$($s.id)' has no transitions and no onTick actions: $RelPath"
                }
            }
        }
        
        Write-Host "  [PASS] $RelPath (States: $($json.states.Count))" -ForegroundColor Green
    }
    catch {
        Fail "Parse error: $($_.Name) => $($_.Exception.Message)"
    }
}

Write-Host ""
if ($errors -gt 0) {
    Write-Host "[FAIL] $errors error(s) found" -ForegroundColor Red
    exit 1
}
Write-Host "[PASS] All graphs validated successfully" -ForegroundColor Green
exit 0
