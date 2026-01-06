<#
.SYNOPSIS
    Validates AI JSON profiles against the canonical EAIS schema.
    
.DESCRIPTION
    This script validates:
    1. Runtime JSON profiles (*.runtime.json) - must NOT contain editor metadata
    2. Editor JSON profiles (*.editor.json) - must contain correct schema

.PARAMETER ProfilesDir
    Directory containing *.runtime.json files

.PARAMETER EditorDir
    Directory containing *.editor.json files

.EXAMPLE
    .\ValidateAIJson.ps1
    .\ValidateAIJson.ps1 -ProfilesDir "Content/AIProfiles"
#>

param(
    [string]$ProfilesDir = "$PSScriptRoot/../../Content/AIProfiles",
    [string]$EditorDir = "$PSScriptRoot/../../Editor/AI"
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

# Forbidden keys in runtime json
$RuntimeForbiddenTop = @("editor", "schemaVersion", "viewport", "nodes", "edges")
$RuntimeForbiddenState = @("pos", "color", "comment")

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ValidateAIJson.ps1 - EAIS JSON Validator" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "ProfilesDir: $ProfilesDir" -ForegroundColor Gray
Write-Host "EditorDir: $EditorDir" -ForegroundColor Gray
Write-Host ""

# Normalize paths early (and fail fast if missing)
try {
    if (-not (Test-Path $ProfilesDir)) {
        Fail "Profiles directory not found: $ProfilesDir"
    }
    else {
        $ProfilesDir = (Resolve-Path $ProfilesDir).Path
    }

    if (-not (Test-Path $EditorDir)) {
        Fail "Editor directory not found: $EditorDir"
    }
    else {
        $EditorDir = (Resolve-Path $EditorDir).Path
    }
}
catch {
    Fail "Path normalization error: $($_.Exception.Message)"
}

# 1) Validate editor JSON schema
if (Test-Path $EditorDir) {
    Write-Host "Validating Editor JSON files..." -ForegroundColor Yellow
    Get-ChildItem -Path $EditorDir -Recurse -Filter "*.editor.json" | ForEach-Object {
        try {
            $j = Get-Content $_.FullName -Raw | ConvertFrom-Json
            $RelPath = $_.FullName.Replace((Resolve-Path $EditorDir).Path, "")
            
            # Top-level required keys
            $missingKeys = @()
            foreach ($k in @("schemaVersion", "name", "initialState", "states", "editor")) {
                if (-not (HasKey $j $k)) { $missingKeys += $k }
            }
            if ($missingKeys.Count -gt 0) {
                Fail "Editor JSON missing keys [$($missingKeys -join ', ')]: $RelPath"
                return
            }

            # No extra top-level keys (strict)
            $allowed = @("schemaVersion", "name", "initialState", "states", "editor")
            foreach ($p in $j.PSObject.Properties.Name) {
                if ($allowed -notcontains $p) { 
                    Fail "Editor JSON has forbidden top-level key [$p]: $RelPath"
                }
            }

            # states must be array
            if ($j.states -is [System.Collections.IDictionary]) { 
                Fail "Editor JSON states must be ARRAY: $RelPath"
                return
            }
            if ($j.states.Count -lt 1) { 
                Fail "Editor JSON states empty: $RelPath"
                return
            }

            # Validate state objects and ensure NO inline layout keys
            $stateIds = @{}
            foreach ($s in $j.states) {
                if (-not (HasKey $s "id") -or [string]::IsNullOrWhiteSpace($s.id)) {
                    Fail "State missing valid 'id' in $RelPath"
                    break
                }
                if ($stateIds.ContainsKey($s.id)) {
                    Fail "Duplicate state ID found: '$($s.id)' in $RelPath"
                }
                $stateIds[$s.id] = $true

                foreach ($k in @("id", "terminal", "onEnter", "onTick", "onExit", "transitions")) {
                    if (-not (HasKey $s $k)) { 
                        Fail "State '$($s.id)' missing [$k] in $RelPath"
                        break 
                    }
                }

                foreach ($bad in $RuntimeForbiddenState) {
                    if (HasKey $s $bad) { 
                        Fail "Editor JSON state illegally contains layout key [$bad]; must be under editor.nodes: $RelPath"
                    }
                }
            }

            # Validate editor.nodes layout map
            if (-not (HasKey $j.editor "nodes")) { 
                Fail "Editor JSON editor.nodes missing: $RelPath"
                return
            }
            $nodeMap = $j.editor.nodes
            if ($nodeMap -isnot [System.Management.Automation.PSCustomObject]) { 
                Fail "editor.nodes must be an object map: $RelPath"
            }

            # Ensure every state has a node layout entry
            foreach ($id in $stateIds.Keys) {
                if (-not (HasKey $nodeMap $id)) {
                    Fail "Missing editor.nodes layout for state [$id]: $RelPath"
                }
                else {
                    $n = $nodeMap.$id
                    if (-not (HasKey $n "pos")) { 
                        Fail "Missing editor.nodes[$id].pos: $RelPath"
                    }
                    elseif (-not (HasKey $n.pos "x") -or -not (HasKey $n.pos "y")) { 
                        Fail "Missing pos.x/pos.y for [$id]: $RelPath"
                    }
                }
            }

            # Transition target existence check (editor)
            foreach ($s in $j.states) {
                if (HasKey $s "transitions") {
                    foreach ($t in $s.transitions) {
                        if (HasKey $t "target") {
                            $targetState = $t.target
                            if (-not [string]::IsNullOrWhiteSpace($targetState) -and -not $stateIds.ContainsKey($targetState)) {
                                Fail "Transition in state '$($s.id)' targets non-existent state: '$targetState' in $RelPath"
                            }
                        }
                    }
                }
            }
            
            Write-Host "  [PASS] $RelPath" -ForegroundColor Green
        }
        catch {
            Fail "Editor JSON parse error: $($_.FullName) => $($_.Exception.Message)"
        }
    }
}
else {
    Write-Host "Editor directory not found: $EditorDir" -ForegroundColor Yellow
}

Write-Host ""

# 2) Validate that runtime JSON files contain NO editor-only metadata
if (Test-Path $ProfilesDir) {
    Write-Host "Validating Runtime JSON files..." -ForegroundColor Yellow
    Get-ChildItem -Path $ProfilesDir -Recurse -Filter "*.runtime.json" | ForEach-Object {
        try {
            $j = Get-Content $_.FullName -Raw | ConvertFrom-Json
            $RelPath = $_.FullName.Replace((Resolve-Path $ProfilesDir).Path, "")
            $hasError = $false

            foreach ($bad in $RuntimeForbiddenTop) {
                if (HasKey $j $bad) { 
                    Fail "Runtime JSON contains forbidden top-level key [$bad]: $RelPath"
                    $hasError = $true
                }
            }

            # Validate required runtime keys
            if (-not (HasKey $j "name")) {
                Fail "Runtime JSON missing 'name': $RelPath"
                $hasError = $true
            }
            if (-not (HasKey $j "initialState")) {
                Fail "Runtime JSON missing 'initialState': $RelPath"
                $hasError = $true
            }
            if (-not (HasKey $j "states")) {
                Fail "Runtime JSON missing 'states': $RelPath"
                $hasError = $true
            }

            if (HasKey $j "states") {
                if ($j.states -isnot [array]) {
                    Fail "Runtime JSON 'states' must be array: $RelPath"
                    $hasError = $true
                }
                else {
                    # --- State ID Uniqueness Check ---
                    $stateIds = @{}
                    foreach ($state in $j.states) {
                        if (-not (HasKey $state "id") -or [string]::IsNullOrWhiteSpace($state.id)) {
                            Fail "Runtime JSON state missing valid 'id': $RelPath"
                            $hasError = $true
                            continue
                        }
                        $id = $state.id
                        if ($stateIds.ContainsKey($id)) {
                            Fail "Duplicate state ID found: '$id' in $RelPath"
                            $hasError = $true
                        }
                        else {
                            $stateIds[$id] = $true
                        }
                    }

                    # initialState must exist
                    if (HasKey $j "initialState") {
                        $initial = $j.initialState
                        if (-not [string]::IsNullOrWhiteSpace($initial) -and -not $stateIds.ContainsKey($initial)) {
                            Fail "Runtime JSON initialState '$initial' does not match any state id: $RelPath"
                            $hasError = $true
                        }
                    }

                    foreach ($s in $j.states) {
                        foreach ($bad in $RuntimeForbiddenState) {
                            if (HasKey $s $bad) { 
                                Fail "Runtime JSON contains forbidden state key [$bad]: $RelPath"
                                $hasError = $true
                            }
                        }
                        
                        # Check transitions have priority
                        if (HasKey $s "transitions") {
                            foreach ($t in $s.transitions) {
                                if (-not (HasKey $t "priority")) {
                                    Fail "Runtime JSON transition missing 'priority' in state '$($s.id)': $RelPath"
                                    $hasError = $true
                                }
                                elseif ($t.priority -isnot [int]) {
                                    Fail "Runtime JSON transition priority must be int in state '$($s.id)': $RelPath"
                                    $hasError = $true
                                }

                                # --- Transition Target Existence Check ---
                                if (HasKey $t "target") {
                                    $targetState = $t.target
                                    if (-not [string]::IsNullOrWhiteSpace($targetState) -and -not $stateIds.ContainsKey($targetState)) {
                                        Fail "Transition in state '$($s.id)' targets non-existent state: '$targetState' in $RelPath"
                                        $hasError = $true
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            if (-not $hasError) {
                Write-Host "  [PASS] $RelPath" -ForegroundColor Green
            }
        }
        catch {
            Fail "Runtime JSON parse error: $($_.FullName) => $($_.Exception.Message)"
        }
    }
}
else {
    Write-Host "Profiles directory not found: $ProfilesDir" -ForegroundColor Yellow
}

Write-Host ""
if ($errors -gt 0) { 
    Write-Host "[FAIL] $errors error(s) found" -ForegroundColor Red
    exit 1 
}
Write-Host "[PASS] All JSON files validated successfully" -ForegroundColor Green
exit 0
