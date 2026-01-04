# EAIS JSON Schema Documentation

## Overview

EAIS uses two JSON formats:
1. **Runtime JSON** (`*.runtime.json`) - Loaded at runtime
2. **Editor JSON** (`*.editor.json`) - Used by visual editor only

## File Locations

| Type | Location |
|------|----------|
| Runtime JSON | `Content/AIProfiles/*.runtime.json` |
| Editor JSON | `Editor/AI/*.editor.json` |

---

## Runtime JSON Schema

### Top-Level Structure
```json
{
  "name": "string",
  "initialState": "string",
  "blackboard": [ /* FEAISBlackboardEntry[] */ ],
  "states": [ /* FAIState[] */ ]
}
```

### Blackboard Entry
```json
{
  "key": "string",
  "value": {
    "type": "Bool|Int|Float|String|Vector",
    "rawValue": "string"
  }
}
```

### State
```json
{
  "id": "string",
  "terminal": false,
  "onEnter": [ /* FAIActionEntry[] */ ],
  "onTick": [ /* FAIActionEntry[] */ ],
  "onExit": [ /* FAIActionEntry[] */ ],
  "transitions": [ /* FAITransition[] */ ]
}
```

### Action Entry
```json
{
  "actionName": "string",
  "paramsJson": "{ ... }"
}
```

### Transition
```json
{
  "to": "StateId",
  "priority": 100,
  "condition": {
    "type": "Blackboard|Event|Timer|Distance",
    "keyOrName": "string",
    "op": "Equal|NotEqual|GreaterThan|LessThan|GreaterOrEqual|LessOrEqual",
    "compareValue": {
      "type": "Bool|Int|Float|String",
      "rawValue": "string"
    },
    "seconds": 0.0,
    "target": "string"
  }
}
```

---

## Editor JSON Schema

Editor JSON includes all runtime fields PLUS editor-only metadata.

### Top-Level Structure
```json
{
  "schemaVersion": 1,
  "name": "string",
  "initialState": "string",
  "states": [ /* same as runtime */ ],
  "editor": {
    "nodes": { /* StateId -> NodeMeta */ },
    "edges": [ /* EdgeMeta[] */ ],
    "viewport": { "zoom": 1.0, "pan": { "x": 0, "y": 0 } }
  }
}
```

### Node Metadata
```json
{
  "StateId": {
    "pos": { "x": 100.0, "y": 200.0 },
    "color": "#3A3A3A",
    "comment": "Optional comment"
  }
}
```

### Edge Metadata
```json
{
  "from": "StateId",
  "to": "StateId",
  "comment": "Optional"
}
```

---

## Forbidden Keys

### Runtime JSON MUST NOT contain:
- `editor`
- `schemaVersion`
- `viewport`
- `nodes`
- `edges`
- `pos`
- `color`
- `comment`

### Validation
Use `ValidateAIJson.ps1` to verify:
```powershell
.\Scripts\ValidateAIJson.ps1
```

---

## Condition Types

| Type | Description | Required Fields |
|------|-------------|-----------------|
| `Blackboard` | Check blackboard value | `keyOrName`, `op`, `compareValue` |
| `Event` | Check for event | `keyOrName` |
| `Timer` | Check elapsed time | `seconds` |
| `Distance` | Check distance to target | `target`, `op`, `compareValue` |

---

## Operator Types

| Operator | Symbol | Description |
|----------|--------|-------------|
| `Equal` | `==` | Values are equal |
| `NotEqual` | `!=` | Values are not equal |
| `GreaterThan` | `>` | Left > Right |
| `LessThan` | `<` | Left < Right |
| `GreaterOrEqual` | `>=` | Left >= Right |
| `LessOrEqual` | `<=` | Left <= Right |

---

## Example: Complete Runtime JSON

```json
{
  "name": "ExampleAI",
  "initialState": "Idle",
  "blackboard": [
    { "key": "IsActive", "value": { "type": "Bool", "rawValue": "false" } },
    { "key": "Health", "value": { "type": "Float", "rawValue": "100.0" } }
  ],
  "states": [
    {
      "id": "Idle",
      "terminal": false,
      "onEnter": [
        { "actionName": "Log", "paramsJson": "{ \"message\": \"Entering Idle\" }" }
      ],
      "onTick": [],
      "onExit": [],
      "transitions": [
        {
          "to": "Active",
          "priority": 100,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "IsActive",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        }
      ]
    },
    {
      "id": "Active",
      "terminal": false,
      "onEnter": [],
      "onTick": [
        { "actionName": "DoWork", "paramsJson": "{}" }
      ],
      "onExit": [],
      "transitions": [
        {
          "to": "Idle",
          "priority": 100,
          "condition": {
            "type": "Timer",
            "seconds": 5.0
          }
        }
      ]
    }
  ]
}
```

---

*P_EAIS - Modular AI System*
