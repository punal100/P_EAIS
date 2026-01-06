# EAIS Automation & CI Documentation

## Overview

P_EAIS includes comprehensive automation for building, testing, and validating AI profiles.

## Scripts

### 1. Main Pipeline: `RunEAISTests.ps1`

The master validation script that runs all checks.

```powershell
# Basic validation (no build)
.\DevTools\scripts\RunEAISTests.ps1 -SkipBuild

# Full run with tests
.\DevTools\scripts\RunEAISTests.ps1 -RunTests -VerboseOutput

# Profile-only validation
.\DevTools\scripts\ValidateAIJson.ps1
.\DevTools\scripts\ValidateGraph.ps1
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
.\DevTools\scripts\ValidateAIJson.ps1
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
.\DevTools\scripts\ValidateGraph.ps1
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
.\DevTools\scripts\VerifyPlanConsistency.ps1
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

### test_eais.bat

Convenience wrapper that calls `RunEAISTests.ps1` on Windows.

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

`DevTools/scripts/EAIS_ActionManifest.json` defines registered actions:

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
.\DevTools\scripts\RunEAISTests.ps1 -SkipBuild

# 2. Build
.\DevTools\scripts\build_headless.bat

# 3. Run tests
.\DevTools\scripts\RunEAISTests.ps1 -SkipBuild -RunTests
```

### Quick Validation:

```powershell
.\DevTools\scripts\ValidateAIJson.ps1
.\DevTools\scripts\ValidateGraph.ps1
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

_P_EAIS - Modular AI System_
