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

## 📁 Profile Locations

AI profiles are stored in two locations:

| Type | Extension | Path | Purpose |
|------|-----------|------|---------|
| Runtime | `*.runtime.json` | `Plugins/P_EAIS/Content/AIProfiles/` | Used at runtime by AI components |
| Editor | `*.editor.json` | `Plugins/P_EAIS/Editor/AI/` | Contains editor metadata (node positions) |

**Note:** The AI Editor widgets display the resolved absolute paths in their UI, making it easy to see where files should be placed.

### Path Search Order

The editors search for profiles in this order:
1. `[ProjectPlugins]/P_EAIS/Content/AIProfiles/` (or `Editor/AI/` for editor profiles)
2. `[Project]/Plugins/P_EAIS/Content/AIProfiles/` (git submodule path)
3. `[ProjectContent]/AIProfiles/` (runtime only, fallback)

### Custom Profile Locations

You can configure additional AI profile search paths in your project's `DefaultGame.ini`:

```ini
[/Script/P_EAIS.EAISSettings]
+AdditionalProfilePaths=(Path="../Plugins/MyPlugin/Content/AIProfiles")
```

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

## 🎯 Modular Integration Examples

P_EAIS is designed to be completely game-agnostic. Below are comprehensive examples for integrating into ANY Unreal Engine project.

### Example 1: Guard AI (Patrol and Chase)

Create `Content/AIProfiles/Guard.runtime.json`:

```json
{
  "name": "Guard",
  "initialState": "Patrol",
  "blackboard": [
    { "key": "PatrolIndex", "value": { "type": "Int", "rawValue": "0" } },
    { "key": "EnemyVisible", "value": { "type": "Bool", "rawValue": "false" } }
  ],
  "states": [
    {
      "id": "Patrol",
      "terminal": false,
      "onEnter": [{ "actionName": "Log", "paramsJson": "{ \"message\": \"Patrolling\" }" }],
      "onTick": [{ "actionName": "MoveTo", "paramsJson": "{ \"target\": \"CurrentWaypoint\" }" }],
      "onExit": [],
      "transitions": [
        {
          "to": "Chase",
          "priority": 100,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "EnemyVisible",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        },
        {
          "to": "NextWaypoint",
          "priority": 50,
          "condition": {
            "type": "Distance",
            "target": "CurrentWaypoint",
            "op": "LessThan",
            "compareValue": { "type": "Float", "rawValue": "100.0" }
          }
        }
      ]
    },
    {
      "id": "NextWaypoint",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"IncrementPatrol\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Patrol", "priority": 100, "condition": { "type": "Timer", "seconds": 0.1 } }
      ]
    },
    {
      "id": "Chase",
      "terminal": false,
      "onEnter": [{ "actionName": "Log", "paramsJson": "{ \"message\": \"Enemy spotted!\" }" }],
      "onTick": [{ "actionName": "MoveTo", "paramsJson": "{ \"target\": \"EnemyPosition\" }" }],
      "onExit": [],
      "transitions": [
        {
          "to": "Attack",
          "priority": 100,
          "condition": {
            "type": "Distance",
            "target": "EnemyPosition",
            "op": "LessThan",
            "compareValue": { "type": "Float", "rawValue": "200.0" }
          }
        },
        {
          "to": "Patrol",
          "priority": 50,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "EnemyVisible",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "false" }
          }
        }
      ]
    },
    {
      "id": "Attack",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"PerformAttack\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Chase", "priority": 100, "condition": { "type": "Timer", "seconds": 1.0 } }
      ]
    }
  ]
}
```

### Example 2: Shopkeeper NPC (Event-Driven)

```json
{
  "name": "Shopkeeper",
  "initialState": "Idle",
  "blackboard": [
    { "key": "PlayerNearby", "value": { "type": "Bool", "rawValue": "false" } }
  ],
  "states": [
    {
      "id": "Idle",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"PlayIdleAnim\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Greeting", "priority": 100, "condition": { "type": "Event", "keyOrName": "PlayerInteract" } },
        {
          "to": "Wave",
          "priority": 50,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "PlayerNearby",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        }
      ]
    },
    {
      "id": "Wave",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"PlayWaveAnim\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Greeting", "priority": 100, "condition": { "type": "Event", "keyOrName": "PlayerInteract" } },
        { "to": "Idle", "priority": 50, "condition": { "type": "Timer", "seconds": 3.0 } }
      ]
    },
    {
      "id": "Greeting",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"OpenDialogue\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "ShowShop", "priority": 100, "condition": { "type": "Event", "keyOrName": "BrowseShop" } },
        { "to": "Idle", "priority": 50, "condition": { "type": "Event", "keyOrName": "DialogueEnd" } }
      ]
    },
    {
      "id": "ShowShop",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"OpenShopUI\" }" }],
      "onTick": [],
      "onExit": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"CloseShopUI\" }" }],
      "transitions": [
        { "to": "Idle", "priority": 100, "condition": { "type": "Event", "keyOrName": "ShopClosed" } }
      ]
    }
  ]
}
```

### Example 3: Boss AI (Phase-Based Combat)

```json
{
  "name": "BossAI",
  "initialState": "Inactive",
  "blackboard": [
    { "key": "HealthPercent", "value": { "type": "Float", "rawValue": "100" } },
    { "key": "PlayerInArena", "value": { "type": "Bool", "rawValue": "false" } }
  ],
  "states": [
    {
      "id": "Inactive",
      "terminal": false,
      "onEnter": [],
      "onTick": [],
      "onExit": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"PlayBossIntro\" }" }],
      "transitions": [
        {
          "to": "Phase1",
          "priority": 100,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "PlayerInArena",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        }
      ]
    },
    {
      "id": "Phase1",
      "terminal": false,
      "onTick": [{ "actionName": "MoveTo", "paramsJson": "{ \"target\": \"Player\" }" }],
      "onEnter": [],
      "onExit": [],
      "transitions": [
        {
          "to": "Phase2",
          "priority": 200,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "HealthPercent",
            "op": "LessThan",
            "compareValue": { "type": "Float", "rawValue": "50" }
          }
        },
        {
          "to": "Attack",
          "priority": 100,
          "condition": {
            "type": "Distance",
            "target": "Player",
            "op": "LessThan",
            "compareValue": { "type": "Float", "rawValue": "300" }
          }
        }
      ]
    },
    {
      "id": "Attack",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"BasicAttack\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Phase1", "priority": 100, "condition": { "type": "Timer", "seconds": 1.5 } }
      ]
    },
    {
      "id": "Phase2",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"EnterPhase2\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        {
          "to": "Defeated",
          "priority": 300,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "HealthPercent",
            "op": "LessOrEqual",
            "compareValue": { "type": "Float", "rawValue": "0" }
          }
        },
        { "to": "SpecialAttack", "priority": 100, "condition": { "type": "Timer", "seconds": 5.0 } }
      ]
    },
    {
      "id": "SpecialAttack",
      "terminal": false,
      "onEnter": [{ "actionName": "Execute", "paramsJson": "{ \"target\": \"SpecialAttack\" }" }],
      "onTick": [],
      "onExit": [],
      "transitions": [
        { "to": "Phase2", "priority": 100, "condition": { "type": "Timer", "seconds": 2.0 } }
      ]
    },
    {
      "id": "Defeated",
      "terminal": true,
      "onEnter": [
        { "actionName": "Execute", "paramsJson": "{ \"target\": \"PlayDeathAnim\" }" },
        { "actionName": "Execute", "paramsJson": "{ \"target\": \"SpawnLoot\" }" }
      ],
      "onTick": [],
      "onExit": [],
      "transitions": []
    }
  ]
}
```

### C++ Integration Example

```cpp
// MyAICharacter.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EAIS_TargetProvider.h"
#include "MyAICharacter.generated.h"

UCLASS()
class AMyAICharacter : public ACharacter, public IEAIS_TargetProvider
{
    GENERATED_BODY()

public:
    AMyAICharacter();

    UPROPERTY(EditAnywhere, Category = "AI")
    TArray<FVector> PatrolPoints;

protected:
    virtual void BeginPlay() override;
    
    // IEAIS_TargetProvider
    virtual bool EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const override;
    virtual int32 EAIS_GetTeamId_Implementation() const override { return 0; }

private:
    UPROPERTY()
    class UAIComponent* AIComponent;
};

// MyAICharacter.cpp
AMyAICharacter::AMyAICharacter()
{
    AIComponent = CreateDefaultSubobject<UAIComponent>(TEXT("AIComponent"));
}

void AMyAICharacter::BeginPlay()
{
    Super::BeginPlay();
    if (AIComponent)
    {
        AIComponent->JsonFilePath = TEXT("Guard.runtime.json");
        AIComponent->bAutoStart = true;
    }
}

bool AMyAICharacter::EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const
{
    FString Target = TargetId.ToString();
    
    if (Target == TEXT("CurrentWaypoint") && PatrolPoints.Num() > 0)
    {
        int32 Index = AIComponent->GetBlackboardInt(TEXT("PatrolIndex")) % PatrolPoints.Num();
        OutLocation = PatrolPoints[Index];
        return true;
    }
    
    if (Target == TEXT("EnemyPosition"))
    {
        OutLocation = AIComponent->GetBlackboardVector(TEXT("EnemyPosition"));
        return true;
    }
    
    return false;
}
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
