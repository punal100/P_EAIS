<#
.SYNOPSIS
Verifies that AI profile paths are correctly configured.

.DESCRIPTION
Checks that profile directories exist and contain valid JSON files.
#>

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptDir "..\..") ).Path

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "     P_EAIS Profile Path Verification             " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

$RuntimeDir = Join-Path $PluginRoot "Content\AIProfiles"
$EditorDir = Join-Path $PluginRoot "Editor\AI"

$allPassed = $true

# Check runtime directory
Write-Host "`n[1] Runtime Profiles ($RuntimeDir)"
if (Test-Path $RuntimeDir) {
    $runtimeFiles = Get-ChildItem -Path $RuntimeDir -Filter "*.json" -File
    if ($runtimeFiles.Count -gt 0) {
        Write-Host "    [OK] Found $($runtimeFiles.Count) runtime profile(s):" -ForegroundColor Green
        foreach ($file in $runtimeFiles) {
            Write-Host "        - $($file.Name)"
        }
    } else {
        Write-Host "    [WARN] Directory exists but contains no .json files" -ForegroundColor Yellow
    }
} else {
    Write-Host "    [FAIL] Directory does not exist!" -ForegroundColor Red
    $allPassed = $false
}

# Check editor directory
Write-Host "`n[2] Editor Profiles ($EditorDir)"
if (Test-Path $EditorDir) {
    $editorFiles = Get-ChildItem -Path $EditorDir -Filter "*.editor.json" -File
    if ($editorFiles.Count -gt 0) {
        Write-Host "    [OK] Found $($editorFiles.Count) editor profile(s):" -ForegroundColor Green
        foreach ($file in $editorFiles) {
            Write-Host "        - $($file.Name)"
        }
    } else {
        Write-Host "    [WARN] Directory exists but contains no .editor.json files" -ForegroundColor Yellow
    }
} else {
    Write-Host "    [FAIL] Directory does not exist!" -ForegroundColor Red
    $allPassed = $false
}

# Validate JSON syntax
Write-Host "`n[3] JSON Syntax Validation"
$allJsonFiles = @()
if (Test-Path $RuntimeDir) {
    $allJsonFiles += Get-ChildItem -Path $RuntimeDir -Filter "*.json" -File
}
if (Test-Path $EditorDir) {
    $allJsonFiles += Get-ChildItem -Path $EditorDir -Filter "*.json" -File
}

foreach ($file in $allJsonFiles) {
    try {
        $content = Get-Content -Path $file.FullName -Raw
        $null = $content | ConvertFrom-Json
        Write-Host "    [OK] $($file.Name)" -ForegroundColor Green
    } catch {
        Write-Host "    [FAIL] $($file.Name): Invalid JSON syntax" -ForegroundColor Red
        $allPassed = $false
    }
}

# Summary
Write-Host "`n==================================================" -ForegroundColor Cyan
if ($allPassed) {
    Write-Host "     ALL CHECKS PASSED                            " -ForegroundColor Green
    exit 0
} else {
    Write-Host "     SOME CHECKS FAILED                           " -ForegroundColor Red
    exit 1
}
