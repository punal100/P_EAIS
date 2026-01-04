# EAIS Automation & CI Documentation

## Overview

P_EAIS includes comprehensive automation for building, testing, and validating AI profiles.

## Scripts

### 1. Main Pipeline: `TestEAIS.ps1`

The master validation script that runs all checks.

```powershell
# Basic validation (no build)
.\Scripts\TestEAIS.ps1 -SkipBuild

# Full run with tests
.\Scripts\TestEAIS.ps1 -RunTests -VerboseOutput

# Validate specific profile
.\Scripts\TestEAIS.ps1 -Profile "MyProfile" -ValidateOnly
```

**Steps performed:**
1. Project build (optional)
2. AI profile validation
3. Automation tests (optional)
4. Source code analysis
5. Plugin structure verification
6. DevTools verification

---

### 2. JSON Validation: `ValidateAIJson.ps1`

Validates runtime and editor JSON profiles.

```powershell
.\Scripts\ValidateAIJson.ps1
```

**Checks:**
- Runtime JSON has no editor-only keys
- Editor JSON conforms to schema
- All required fields present
- JSON syntax valid

---

### 3. Graph Validation: `ValidateGraph.ps1`

Validates editor graph structure.

```powershell
.\Scripts\ValidateGraph.ps1
```

**Checks:**
- Initial state exists
- All transition targets exist
- Non-terminal states have transitions or onTick
- States array not empty

---

### 4. Plan Consistency: `VerifyPlanConsistency.ps1`

Checks for forbidden legacy patterns.

```powershell
.\Scripts\VerifyPlanConsistency.ps1
```

**Checks:**
- No `TMap<FString, FString> Blackboard` usage
- No object-based states JSON
- Editor code doesn't call FAIInterpreter

---

## DevTools Scripts

Located in `DevTools/scripts/`:

### build_headless.bat
Builds the project without opening the editor.

### run_tests.bat
Runs EAIS automation tests.

### generate_Editor.bat
Generates Editor Utility Widgets via P_MWCS (if available).

---

## CI Configuration

### GitHub Actions: `eais_ci.yml`

Located at `DevTools/ci/eais_ci.yml`.

**Triggers:**
- Push to main
- Pull requests

**Jobs:**
1. Validate JSON profiles
2. Build project
3. Run automation tests
4. Check for forbidden patterns

---

## CI Failure Conditions

**Build MUST FAIL if:**
- Any AI JSON is invalid
- Any state has no outgoing transitions (unless terminal)
- Any action name is unregistered (manifest check)
- Any EAIS file accesses game-specific classes

---

## Action Manifest

`Scripts/EAIS_ActionManifest.json` defines registered actions:

```json
{
  "version": 1,
  "actions": [
    { "name": "Log", "paramsStruct": "FAIActionParams" },
    { "name": "MoveTo", "paramsStruct": "FAIActionParams" },
    { "name": "Wait", "paramsStruct": "FAIActionParams" }
  ]
}
```

---

## Local Development Workflow

### Before Committing:
```powershell
# 1. Run validation
.\Scripts\TestEAIS.ps1 -SkipBuild

# 2. Build
.\DevTools\scripts\build_headless.bat

# 3. Run tests
.\DevTools\scripts\run_tests.bat
```

### Quick Validation:
```powershell
.\Scripts\ValidateAIJson.ps1
.\Scripts\ValidateGraph.ps1
```

---

## Troubleshooting

### "Invalid JSON syntax"
- Check for missing commas, brackets
- Use a JSON validator

### "State has no transitions"
- Add `"terminal": true` if it's an end state
- Add transitions or onTick actions

### "Editor key in runtime JSON"
- Remove `editor`, `pos`, `color`, `comment` keys
- Use ValidateAIJson.ps1 to find violations

### "Unknown action name"
- Register action in EAISSubsystem
- Update EAIS_ActionManifest.json

---

*P_EAIS - Modular AI System*
