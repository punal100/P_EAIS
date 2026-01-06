# P_EAIS Developer Guide

This guide covers the implementation details, extension points, and workflow for the **Enhanced AI System**.

> [!WARNING] > **Work in Progress**: This plugin is under active development. Core functionality is partially working, but some features (especially the Visual AI Editor) may be incomplete or unstable.

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Architecture Overview](#2-architecture-overview)
3. [JSON Schema](#3-json-schema)
4. [Creating AI Profiles](#4-creating-ai-profiles)
5. [Core Components](#5-core-components)
6. [Visual AI Editor](#6-visual-ai-editor)
7. [Custom Actions](#7-custom-actions)
8. [Blackboard System](#8-blackboard-system)
9. [Input Injection](#9-input-injection)
10. [CI/Automation](#11-ciautomation)
11. [Best Practices](#12-best-practices)
12. [Target Resolution & Action Execution](#13-target-resolution--action-execution)

---

## 1. Getting Started

### Prerequisites

- Unreal Engine 5.x
- P_MEIS plugin (optional, for input injection)
- P_MWCS plugin (optional, for EUW generation)

### Quick Setup

1. Enable `P_EAIS` in your project
2. Create an AI profile JSON file
3. Attach `UAIComponent` to your pawn
4. Set the profile path and start AI

```cpp
UAIComponent* AIComp = NewObject<UAIComponent>(this);
AIComp->RegisterComponent();
AIComp->JsonFilePath = TEXT("MyProfile.runtime.json");
AIComp->StartAI();
```

---

## 2. Architecture Overview

### Execution Contract

EAIS operates as a **Deterministic, Server-Authoritative Hybrid FSM/BT Runtime**.

**Rules:**

1. EAIS ticks ONLY on the Server
2. EAIS NEVER directly modifies gameplay state
3. EAIS ONLY injects intent via actions
4. State transitions are deterministic (single-transition-per-tick)
5. EAIS is replay-safe (event order deterministic)

### Component Hierarchy

```
UEAISSubsystem (GameInstanceSubsystem)
    ├── Action Registry
    └── Global Settings

UAIComponent (ActorComponent)
    ├── FAIInterpreter (State Machine Runtime)
    │   ├── FAIBehaviorDef (Parsed JSON)
    │   ├── Blackboard (TArray<FEAISBlackboardEntry>)
    │   └── Event Queue
    └── Delegates (OnStateChanged, OnActionExecuted)

UAIBehaviour (PrimaryDataAsset)
    ├── EmbeddedJson or JsonFilePath
    └── FAIBehaviorDef (Cached parse result)
```

### Execution Flow

1. **Initialization:** `UAIComponent::BeginPlay()` loads JSON
2. **Start:** `UAIComponent::StartAI()` enters initial state
3. **Tick:** `FAIInterpreter::Tick()`:
   - Process queued events (FIFO)
   - Execute OnTick actions
   - Evaluate transitions (priority-sorted)
   - Clear events
4. **Transition:** Execute OnExit → Enter new state → Execute OnEnter

---

## 3. JSON Schema

### File Types

| Type    | Extension        | Location              |
| ------- | ---------------- | --------------------- |
| Runtime | `*.runtime.json` | `Content/AIProfiles/` |
| Editor  | `*.editor.json`  | `Editor/AI/`          |

### Runtime JSON Structure

```json
{
  "name": "ProfileName",
  "initialState": "StateId",
  "blackboard": [
    { "key": "KeyName", "value": { "type": "Bool", "rawValue": "true" } }
  ],
  "states": [
    {
      "id": "StateId",
      "terminal": false,
      "onEnter": [],
      "onTick": [],
      "onExit": [],
      "transitions": []
    }
  ]
}
```

### Action Entry

```json
{
  "actionName": "ActionName",
  "paramsJson": "{ \"param\": \"value\" }"
}
```

### Transition

```json
{
  "to": "TargetStateId",
  "priority": 100,
  "condition": {
    "type": "Blackboard",
    "keyOrName": "KeyName",
    "op": "Equal",
    "compareValue": { "type": "Bool", "rawValue": "true" }
  }
}
```

### Condition Types

| Type         | Description            | Required Fields                   |
| ------------ | ---------------------- | --------------------------------- |
| `Blackboard` | Check blackboard value | `keyOrName`, `op`, `compareValue` |
| `Event`      | Check for event        | `keyOrName`                       |
| `Timer`      | Check elapsed time     | `seconds`                         |
| `Distance`   | Check distance         | `target`, `op`, `compareValue`    |

### Operators

| Operator         | Description   |
| ---------------- | ------------- |
| `Equal`          | Values equal  |
| `NotEqual`       | Values differ |
| `GreaterThan`    | Left > Right  |
| `LessThan`       | Left < Right  |
| `GreaterOrEqual` | Left >= Right |
| `LessOrEqual`    | Left <= Right |

### Value Types

| Type     | Example rawValue    |
| -------- | ------------------- |
| `Bool`   | `"true"`, `"false"` |
| `Int`    | `"42"`              |
| `Float`  | `"3.14"`            |
| `String` | `"hello"`           |
| `Vector` | `"100,200,300"`     |

---

## 4. Creating AI Profiles

### Step 1: Create JSON File

Create `Content/AIProfiles/MyAI.runtime.json`:

```json
{
  "name": "MyAI",
  "initialState": "Idle",
  "blackboard": [
    { "key": "Health", "value": { "type": "Float", "rawValue": "100" } }
  ],
  "states": [
    {
      "id": "Idle",
      "terminal": false,
      "onEnter": [
        {
          "actionName": "Log",
          "paramsJson": "{ \"message\": \"Entering Idle\" }"
        }
      ],
      "onTick": [],
      "onExit": [],
      "transitions": [
        {
          "to": "Active",
          "priority": 100,
          "condition": {
            "type": "Event",
            "keyOrName": "Activate"
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

### Step 2: Validate

```powershell
.\DevTools\scripts\ValidateAIJson.ps1
```

### Step 3: Use in Code

```cpp
AIComponent->JsonFilePath = TEXT("MyAI.runtime.json");
AIComponent->StartAI();
```

---

## 5. Core Components

### UAIComponent

```cpp
// Get component
UAIComponent* AIComp = Pawn->FindComponentByClass<UAIComponent>();

// Control
AIComp->StartAI();
AIComp->StopAI();
AIComp->ResetAI();

// Events
AIComp->EnqueueSimpleEvent(TEXT("Activate"));

// Blackboard
AIComp->SetBlackboardBool(TEXT("IsReady"), true);
bool bReady = AIComp->GetBlackboardBool(TEXT("IsReady"));
```

### FAIInterpreter

```cpp
// Force transition
Interpreter.ForceTransition(TEXT("Active"));

// Get current state
FString State = Interpreter.GetCurrentStateId();
```

### UEAISSubsystem

```cpp
UEAISSubsystem* Subsystem = UEAISSubsystem::Get(WorldContextObject);

// Register action
Subsystem->RegisterAction(TEXT("MyAction"), UMyAction::StaticClass());
```

---

## 6. Visual AI Editor

### Opening

```
Tools → EAIS → EAIS AI Editor
```

### Key Classes

| Class                   | Description      |
| ----------------------- | ---------------- |
| `UEAIS_GraphNode`       | State node       |
| `UEAIS_GraphSchema`     | Connection rules |
| `SEAIS_GraphEditor`     | Editor widget    |
| `FEAISJsonEditorParser` | JSON parsing     |

### Editor JSON Format

```json
{
  "schemaVersion": 1,
  "name": "ProfileName",
  "initialState": "Idle",
  "states": [
    /* same as runtime */
  ],
  "editor": {
    "nodes": {
      "Idle": { "pos": { "x": 100, "y": 100 } }
    },
    "viewport": { "zoom": 1.0 }
  }
}
```

---

## 7. Custom Actions

### Creating

```cpp
UCLASS()
class UMyAction : public UAIAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(UAIComponent* Owner, const FAIActionParams& Params) override
    {
        APawn* Pawn = Owner->GetOwnerPawn();

        FString Target = Params.Target;
        float Power = Params.Power;

        // Your logic
    }

    virtual FString GetActionName() const override
    {
        return TEXT("MyAction");
    }
};
```

### Registering

```cpp
UEAISSubsystem* Subsystem = UEAISSubsystem::Get(this);
Subsystem->RegisterAction(TEXT("MyAction"), UMyAction::StaticClass());
```

### Using in JSON

```json
{
  "actionName": "MyAction",
  "paramsJson": "{ \"target\": \"enemy\", \"power\": 0.8 }"
}
```

---

## 8. Blackboard System

### Typed Values

```cpp
struct FEAISBlackboardEntry
{
    FString Key;
    FBlackboardValue Value;
};
```

### Access Methods

```cpp
// Bool
AIComp->SetBlackboardBool("IsReady", true);
bool b = AIComp->GetBlackboardBool("IsReady");

// Float
AIComp->SetBlackboardFloat("Health", 100.0f);
float h = AIComp->GetBlackboardFloat("Health");

// Vector
AIComp->SetBlackboardVector("Target", FVector(1, 2, 3));
FVector v = AIComp->GetBlackboardVector("Target");
```

### Auto-Sync Pattern

```cpp
void AMyCharacter::OnHealthChanged(float NewHealth)
{
    if (UAIComponent* AI = FindComponentByClass<UAIComponent>())
    {
        AI->SetBlackboardFloat("Health", NewHealth);
    }
}
```

---

## 9. Input Injection

### With P_MEIS (Optional)

If P_MEIS is available, AI can inject input:

```cpp
APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
if (PC)
{
    UCPP_BPL_InputBinding::InjectActionStarted(PC, FName("Jump"));
    UCPP_BPL_InputBinding::InjectActionTriggered(PC, FName("Sprint"));
    UCPP_BPL_InputBinding::InjectActionCompleted(PC, FName("Jump"));
}
```

---

## 10. Debugging

### Console Commands

| Command                          | Description   |
| -------------------------------- | ------------- |
| `EAIS.Debug 1`                   | Enable debug  |
| `EAIS.Debug 0`                   | Disable debug |
| `EAIS.SpawnBot <Team> <Profile>` | Spawn AI      |
| `EAIS.InjectEvent * <Event>`     | Inject event  |
| `EAIS.ListActions`               | List actions  |

### Debug Overlay

Shows:

- Current state
- Elapsed time
- Blackboard values
- Event queue

---

## 11. CI/Automation

### Scripts

| Script                      | Purpose         |
| --------------------------- | --------------- |
| `RunEAISTests.ps1`          | Full validation |
| `ValidateAIJson.ps1`        | JSON validation |
| `ValidateGraph.ps1`         | Graph structure |
| `VerifyPlanConsistency.ps1` | Code patterns   |

### Running

```powershell
.\DevTools\scripts\RunEAISTests.ps1 -VerboseOutput
```

### CI Failure Conditions

- Invalid JSON
- State with no transitions (unless terminal)
- Unregistered action name

---

## 12. Best Practices

### JSON Profiles

1. Use canonical array format for states
2. Set explicit priorities on transitions
3. Mark end states as `terminal: true`
4. Validate before committing

### Performance

1. Increase TickInterval for non-critical AI
2. Stagger ticks across frames
3. Keep actions lightweight

### Code Organization

1. Implement `IEAIS_TargetProvider` for targets
2. Register actions in GameMode
3. Use events to decouple systems

### Debugging

1. Enable `EAIS.Debug 1`
2. Check logs (`LogEAIS`)
3. Validate JSON with scripts

---

## 13. Target Resolution & Action Execution

To keep AI profiles game-agnostic, EAIS uses interfaces to communicate with gameplay code.

### 13.1 IEAIS_TargetProvider

Interface for resolving logical target names (e.g., "Ball", "Goal") into world coordinates or actor references.

**Signature:**

```cpp
UINTERFACE(BlueprintType)
class UEAIS_TargetProvider : public UInterface { ... };

class IEAIS_TargetProvider
{
    virtual bool EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const;
    virtual bool EAIS_GetTargetActor_Implementation(FName TargetId, AActor*& OutActor) const;
    virtual int32 EAIS_GetTeamId_Implementation() const;
    virtual FString EAIS_GetRole_Implementation() const;
};
```

**Usage:**
The `MoveTo` and `AimAt` actions will first try to resolve their target parameter via this interface if the Pawn implements it.

### 13.2 IEAIS_ActionExecutor

Interface for executing gameplay-specific actions (e.g., "Shoot", "Pass").

**Signature:**

```cpp
UINTERFACE(BlueprintType)
class UEAIS_ActionExecutor : public UInterface { ... };

class IEAIS_ActionExecutor
{
    virtual FEAIS_ActionResult EAIS_ExecuteAction_Implementation(const FName ActionId, const FString& ParamsJson);
};
```

**Usage:**
The `Execute` action in EAIS acts as a bridge. It looks for this interface on the Pawn or its components and forwards the logical `ActionId` and JSON parameters.

---

## 14. Mini Football Integration

This section documents the `P_MiniFootball`-specific sensors and actions available when using EAIS with the Mini Football game.

### 14.1 Blackboard Keys (Sensors)

The following keys are populated by `AMF_PlayerCharacter::SyncBlackboard()` every tick:

| Key                       | Type   | Description                                     |
| ------------------------- | ------ | ----------------------------------------------- |
| `HasBall`                 | Bool   | True if this character possesses the ball       |
| `TeamHasBall`             | Bool   | True if a teammate has the ball                 |
| `OpponentHasBall`         | Bool   | True if an opponent has the ball                |
| `IsBallLoose`             | Bool   | True if no one possesses the ball               |
| `IsInDanger`              | Bool   | True if an opponent is within 200 units         |
| `HasClearShot`            | Bool   | True if no enemies are in the shot cone to goal |
| `DistToBall`              | Float  | Distance to the ball                            |
| `DistToOpponentGoal`      | Float  | Distance to opponent's goal                     |
| `DistToHome`              | Float  | Distance to formation home position             |
| `DistToNearestOpponent`   | Float  | Distance to closest enemy                       |
| `BallPosition`            | Vector | Current ball world location                     |
| `MyPosition`              | Vector | Current character location                      |
| `OpponentGoalPosition`    | Vector | Opponent goal location                          |
| `HomePosition`            | Vector | Formation home position                         |
| `NearestOpponentPosition` | Vector | Closest enemy location                          |

### 14.2 Available Targets

Targets implemented in `IEAIS_TargetProvider`:

| Target Name       | Returns              |
| ----------------- | -------------------- |
| `Ball`            | Ball actor/location  |
| `Goal_Opponent`   | Opponent's goal      |
| `Goal_Self`       | Own team's goal      |
| `Home`            | Formation position   |
| `BallCarrier`     | Player with ball     |
| `NearestOpponent` | Closest enemy player |

### 14.3 Game Actions

Actions handled by `UMF_EAISActionExecutorComponent`:

| Action ID   | Parameters               | Description                   |
| ----------- | ------------------------ | ----------------------------- |
| `MF.Shoot`  | `{"power": 0.0-1.0}`     | Shoot towards aimed direction |
| `MF.Pass`   | None                     | Pass ball forward             |
| `MF.Tackle` | None                     | Execute tackle animation      |
| `MF.Sprint` | `{"active": true/false}` | Toggle sprint mode            |
| `MF.Face`   | `{"target": "Ball"}`     | Rotate to face target         |
| `MF.Mark`   | None                     | Follow nearest opponent       |

### 14.4 Example: Striker Profile Structure

```json
{
  "name": "Striker",
  "initialState": "Main",
  "states": [
    {
      "id": "Main",
      "transitions": [
        {
          "to": "Tackle",
          "priority": 1000,
          "condition": {
            "keyOrName": "OpponentHasBall",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "SafetyPass",
          "priority": 900,
          "condition": {
            "keyOrName": "IsInDanger",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "Shoot",
          "priority": 800,
          "condition": {
            "keyOrName": "HasClearShot",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "ChaseBall",
          "priority": 700,
          "condition": {
            "keyOrName": "IsBallLoose",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "DribbleToGoal",
          "priority": 600,
          "condition": {
            "keyOrName": "HasBall",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "Support",
          "priority": 500,
          "condition": {
            "keyOrName": "TeamHasBall",
            "op": "Equal",
            "compareValue": true
          }
        },
        {
          "to": "ReturnHome",
          "priority": 100,
          "condition": { "type": "Timer", "seconds": 0.0 }
        }
      ]
    }
  ]
}
```

---

## Further Reading

- [README.md](README.md) - Overview
- [Docs/Architecture.md](Docs/Architecture.md) - System design
- [Docs/JSONSchema.md](Docs/JSONSchema.md) - JSON reference
- [Docs/VisualEditor.md](Docs/VisualEditor.md) - Editor guide

---

_P_EAIS - A modular, game-agnostic AI system_
