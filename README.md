# P_EAIS - Enhanced AI System

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![UE Version](https://img.shields.io/badge/UE-5.x-blue)]()
[![Status](https://img.shields.io/badge/status-Work%20In%20Progress-yellow)]()

> [!WARNING] > **Work in Progress**: This plugin is under active development. Core functionality is partially working, but some features (especially the Visual AI Editor) may be incomplete or unstable.

**P_EAIS** is a modular AI plugin for Unreal Engine 5 that provides a **JSON-programmable AI runtime** and a **Visual AI Editor**.

It implements a **Deterministic, Server-Authoritative Hybrid FSM/BT Runtime**, ensuring AI behavior is transparent, predictable, and replay-safe.

## 🌟 Key Features

- **JSON-First Architecture:** Define AI behaviors in human-readable JSON files
- **Hybrid FSM/BT Runtime:** State machine with Behavior Tree-like patterns
- **Typed Blackboard System:** Per-instance key-value storage with Bool, Int, Float, String, Vector support
- **Input Injection:** AI agents can inject input via P_MEIS (optional dependency)
- **Visual AI Editor:** SGraphEditor-based node editor for creating AI behaviors
- **Headless Automation:** Full CI/CD support with validation scripts
- **Game-Agnostic:** Works with any project via the `IEAIS_TargetProvider` interface

## 📦 Module Structure

```
P_EAIS/
├── Source/
│   ├── P_EAIS/               # Runtime module
│   ├── P_EAIS_Editor/        # Editor module (visual editor)
│   └── P_EAISTools/          # Editor tools (EUW, commandlets)
├── Content/AIProfiles/       # Runtime JSON profiles (*.runtime.json)
├── Editor/AI/                # Editor JSON (*.editor.json)
├── DevTools/                 # CI, output, and scripts
│   ├── ci/                   # CI configuration
│   ├── output/               # Build/test output
│   └── scripts/              # Validation scripts
└── Docs/                     # Documentation
```

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    P_EAIS Architecture                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  JSON Profile (.runtime.json)                                │
│       │                                                      │
│       ▼                                                      │
│  UAIBehaviour (UObject) ──► FAIInterpreter (State Machine)  │
│       │                           │                          │
│       ▼                           ▼                          │
│  UAIComponent ◄──────────── Tick / Execute Actions           │
│       │                           │                          │
│       ▼                           ▼                          │
│  IEAIS_TargetProvider ──► Input Injection (optional)        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### 1. Enable Plugin

Add P_EAIS to your project's `.uproject` file or enable via Edit → Plugins.

### 2. Add AI to a Pawn

```cpp
// In your Pawn or Controller
UAIComponent* AIComp = NewObject<UAIComponent>(this);
AIComp->RegisterComponent();
AIComp->JsonFilePath = TEXT("MyProfile.runtime.json");
AIComp->bAutoStart = true;
```

### 3. Create an AI Profile

Create `Content/AIProfiles/MyProfile.runtime.json`:

```json
{
  "name": "MyProfile",
  "initialState": "Idle",
  "blackboard": [
    { "key": "IsActive", "value": { "type": "Bool", "rawValue": "false" } }
  ],
  "states": [
    {
      "id": "Idle",
      "terminal": false,
      "onEnter": [],
      "onTick": [
        { "actionName": "Log", "paramsJson": "{ \"message\": \"Idle\" }" }
      ],
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
        { "actionName": "Log", "paramsJson": "{ \"message\": \"Active\" }" }
      ],
      "onExit": [],
      "transitions": []
    }
  ]
}
```

### 4. Open the Visual Editor

```
Tools → EAIS → EAIS AI Editor
```

## 📐 JSON Schema

### States Array Format

```json
{
  "id": "StateId",
  "terminal": false,
  "onEnter": [
    /* actions */
  ],
  "onTick": [
    /* actions */
  ],
  "onExit": [
    /* actions */
  ],
  "transitions": [
    /* transitions */
  ]
}
```

### Action Format

```json
{
  "actionName": "ActionName",
  "paramsJson": "{ \"param\": \"value\" }"
}
```

### Transition Format

```json
{
  "to": "TargetStateId",
  "priority": 100,
  "condition": {
    "type": "Blackboard|Event|Timer|Distance",
    "keyOrName": "KeyName",
    "op": "Equal|NotEqual|GreaterThan|LessThan",
    "compareValue": { "type": "Bool|Int|Float|String", "rawValue": "value" }
  }
}
```

## 🎨 Visual AI Editor

The Visual AI Editor provides a node-based interface using Unreal's SGraphEditor.

| Class                   | Description         |
| ----------------------- | ------------------- |
| `UEAIS_GraphNode`       | State node in graph |
| `UEAIS_GraphSchema`     | Connection rules    |
| `SEAIS_GraphEditor`     | Main editor widget  |
| `FEAISJsonEditorParser` | JSON parsing        |

## 🧪 Console Commands

| Command                           | Description             |
| --------------------------------- | ----------------------- |
| `EAIS.SpawnBot <Team> <Profile>`  | Spawn AI bot            |
| `EAIS.Debug <0\|1>`               | Toggle debug mode       |
| `EAIS.InjectEvent <Name> <Event>` | Inject event            |
| `EAIS.ListActions`                | List registered actions |

## ✅ Validation Scripts

```powershell
# Full validation pipeline
.\DevTools\scripts\RunEAISTests.ps1 -VerboseOutput

# Individual scripts
.\DevTools\scripts\ValidateAIJson.ps1
.\DevTools\scripts\ValidateGraph.ps1
.\DevTools\scripts\VerifyPlanConsistency.ps1
```

## 🔌 Game Integration

### Implementing IEAIS_TargetProvider

Your game pawn should implement `IEAIS_TargetProvider` for target resolution:

```cpp
class AMyCharacter : public ACharacter, public IEAIS_TargetProvider
{
    virtual bool EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const override
    {
        // Resolve game-specific targets
        if (TargetId == "enemy")
        {
            OutLocation = GetNearestEnemy()->GetActorLocation();
            return true;
        }
        return false;
    }
};
```

## 🏗️ Built-in Actions

| Action             | Description          | Parameters          |
| ------------------ | -------------------- | ------------------- |
| `MoveTo`           | Navigate to target   | `target`, `speed`   |
| `Wait`             | Passive wait         | `seconds`           |
| `Log`              | Debug logging        | `message`           |
| `SetBlackboardKey` | Update blackboard    | `key`, `value`      |
| `Execute`          | Bridge to game logic | `target` (ActionId) |
| `LookAround`       | Reset AI focus       | none                |

## 🔧 Custom Actions

```cpp
UCLASS()
class UMyAction : public UAIAction
{
    GENERATED_BODY()
public:
    virtual void Execute_Implementation(UAIComponent* Owner, const FAIActionParams& Params) override
    {
        // Your logic here
    }

    virtual FString GetActionName() const override { return TEXT("MyAction"); }
};
```

## 📚 Documentation

| Document                                     | Description           |
| -------------------------------------------- | --------------------- |
| [GUIDE.md](GUIDE.md)                         | Developer guide       |
| [Docs/Architecture.md](Docs/Architecture.md) | System architecture   |
| [Docs/JSONSchema.md](Docs/JSONSchema.md)     | JSON schema reference |
| [Docs/VisualEditor.md](Docs/VisualEditor.md) | Visual editor guide   |

## 🔗 Optional Dependencies

| Plugin | Purpose                              |
| ------ | ------------------------------------ |
| P_MEIS | Input injection (AI presses buttons) |
| P_MWCS | Editor Utility Widget generation     |

## 📄 License

MIT License - Punal Manalan

---

_A modular, game-agnostic AI system for Unreal Engine 5_
