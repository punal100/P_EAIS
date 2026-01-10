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
12. [Troubleshooting](#13-troubleshooting)
13. [Target Resolution & Action Execution](#14-target-resolution--action-execution)
14. [Game Integration Patterns](#15-game-integration-patterns)

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

## 4. Configuration

### Multi-Project Profile Loading

EAIS supports loading profiles from external locations (outside P_EAIS plugin directory) in two ways:

**1. Programmatic Loading (Recommended for Plugins):**

When initializing AI from another plugin (e.g. `P_MiniFootball`), you can pass the base directory explicitly to `StartAI`. This is robust and doesn't require INI configuration.

```cpp
// In your Character::BeginPlay
FString PluginDir = IPluginManager::Get().FindPlugin("MyGamePlugin")->GetContentDir();
FString AIProfileDir = FPaths::Combine(PluginDir, TEXT("AIProfiles"));

// Pass the profile name and the explicit search path
AIComponent->StartAI("MyProfile", AIProfileDir);
```

**2. Config-based Loading (Legacy/Editor):**

To allow the Editor or default loading logic to find profiles in other paths, add them to `DefaultGame.ini`:

```ini
[/Script/P_EAIS.EAISSettings]
+AdditionalProfilePaths=(Path="../Plugins/MyPlugin/Content/AIProfiles")
```

## Creating a Profiles

### Step 1: Create JSON File

Create `Content/AIProfiles/MyAI.runtime.json`.

> [!TIP]
> See **P_MiniFootball** for concrete examples:
> - `Striker.runtime.json` - Attack & Shooting
> - `Midfielder.runtime.json` - Support & Transition
> - `Defender.runtime.json` - Marking & Clearing
> - `Goalkeeper.runtime.json` - Goal Protection


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

## 13. Troubleshooting

### 13.1 "Failed to load file" Error

**Symptom:** When validating or loading a profile, you see "Validation failed: Failed to load file"

**Causes & Solutions:**

1. **Profile file doesn't exist at expected path**
   - Check the path displayed in the AI Editor widget toolbar
   - The widget shows absolute paths (e.g., `D:\Projects\...\P_EAIS\Content\AIProfiles\`)
   - Ensure file exists with correct extension (`.runtime.json` or `.editor.json`)

2. **Using wrong file extension**
   - Runtime profiles: `ProfileName.runtime.json`
   - Editor profiles: `ProfileName.editor.json`
   - The system will try both extensions automatically

3. **Relative path not resolved**
   - Fixed in version 3 of the editors
   - Paths are now converted to absolute using `FPaths::ConvertRelativePathToFull()`

### 13.2 Profile Not Appearing in Dropdown

1. Verify file extension is `.runtime.json`, `.editor.json`, or `.json`
2. Check the path displayed in the widget toolbar
3. Click "Refresh" to rescan directories
4. Check Output Log for "Found X profiles" messages

### 13.3 Profile Path Discovery

The AI editors search for profiles in this order:

| Priority | Runtime Path | Editor Path |
|----------|-------------|-------------|
| 1 | `[ProjectPlugins]/P_EAIS/Content/AIProfiles/` | `[ProjectPlugins]/P_EAIS/Editor/AI/` |
| 2 | `[Project]/Plugins/P_EAIS/Content/AIProfiles/` | `[Project]/Plugins/P_EAIS/Editor/AI/` |
| 3 | `[ProjectContent]/AIProfiles/` | N/A |

### 13.4 Verification Script

Run the profile verification script:
```powershell
.\DevTools\scripts\VerifyProfilePaths.ps1
```

This script checks:
- Directory existence
- Profile file count
- JSON syntax validation

Example output:
```
[1] Runtime Profiles (D:\...\P_EAIS\Content\AIProfiles)
    [OK] Found 1 runtime profile(s):
        - Striker.runtime.json

[2] Editor Profiles (D:\...\P_EAIS\Editor\AI)
    [OK] Found 1 editor profile(s):
        - Striker.editor.json

[3] JSON Syntax Validation
    [OK] Striker.runtime.json
    [OK] Striker.editor.json
```

---

## 14. Target Resolution & Action Execution

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

## 15. Game Integration Patterns

This section provides reusable patterns for integrating EAIS into ANY game project.

### 14.1 Sensor Pattern (Blackboard Sync)

Your game character should periodically sync game state to the AI's blackboard:

```cpp
void AMyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    SyncBlackboard();
}

void AMyCharacter::SyncBlackboard()
{
    if (!AIComponent) return;
    
    // Sync health
    AIComponent->SetBlackboardFloat(TEXT("Health"), CurrentHealth);
    AIComponent->SetBlackboardFloat(TEXT("HealthPercent"), (CurrentHealth / MaxHealth) * 100.f);
    
    // Sync combat state
    AIComponent->SetBlackboardBool(TEXT("IsInCombat"), bIsInCombat);
    AIComponent->SetBlackboardInt(TEXT("AmmoCount"), CurrentAmmo);
    
    // Sync detection
    if (AActor* Enemy = GetNearestEnemy())
    {
        AIComponent->SetBlackboardFloat(TEXT("DistToEnemy"), 
            FVector::Dist(GetActorLocation(), Enemy->GetActorLocation()));
        AIComponent->SetBlackboardVector(TEXT("EnemyPosition"), 
            Enemy->GetActorLocation());
        AIComponent->SetBlackboardBool(TEXT("EnemyVisible"), true);
    }
    else
    {
        AIComponent->SetBlackboardBool(TEXT("EnemyVisible"), false);
    }
    
    // Sync position
    AIComponent->SetBlackboardVector(TEXT("MyPosition"), GetActorLocation());
}
```

### 14.2 Target Resolution Pattern

Implement `IEAIS_TargetProvider` to convert logical target names to world positions:

```cpp
bool AMyCharacter::EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const
{
    FString Target = TargetId.ToString();
    
    // Static targets
    if (Target == TEXT("Home") || Target == TEXT("SpawnPoint"))
    {
        OutLocation = SpawnLocation;
        return true;
    }
    
    // Dynamic targets
    if (Target == TEXT("NearestEnemy"))
    {
        if (AActor* Enemy = GetNearestEnemy())
        {
            OutLocation = Enemy->GetActorLocation();
            return true;
        }
        return false;
    }
    
    if (Target == TEXT("NearestCover"))
    {
        if (AActor* Cover = FindNearestCover())
        {
            OutLocation = Cover->GetActorLocation();
            return true;
        }
        return false;
    }
    
    // Blackboard vector targets
    if (Target == TEXT("EnemyPosition") || Target == TEXT("TargetPosition"))
    {
        if (AIComponent)
        {
            OutLocation = AIComponent->GetBlackboardVector(Target);
            return true;
        }
    }
    
    return false;
}
```

### 14.3 Action Execution Pattern

For game-specific actions, implement `IEAIS_ActionExecutor`:

```cpp
FEAIS_ActionResult UMyActionExecutor::EAIS_ExecuteAction_Implementation(
    const FName ActionId, 
    const FString& ParamsJson)
{
    FEAIS_ActionResult Result;
    Result.bSuccess = false;
    
    // Parse parameters
    TSharedPtr<FJsonObject> Params = ParseJsonParams(ParamsJson);
    FString ActionStr = ActionId.ToString();
    
    if (ActionStr == TEXT("Attack"))
    {
        float Power = 1.0f;
        if (Params.IsValid()) Params->TryGetNumberField(TEXT("power"), Power);
        PerformAttack(Power);
        Result.bSuccess = true;
    }
    else if (ActionStr == TEXT("Reload"))
    {
        if (CanReload())
        {
            StartReload();
            Result.bSuccess = true;
        }
    }
    else if (ActionStr == TEXT("UseAbility"))
    {
        int32 AbilityIndex = 0;
        if (Params.IsValid()) Params->TryGetNumberField(TEXT("index"), AbilityIndex);
        Result.bSuccess = ActivateAbility(AbilityIndex);
    }
    else if (ActionStr == TEXT("PlayAnimation"))
    {
        FString AnimName;
        if (Params.IsValid() && Params->TryGetStringField(TEXT("name"), AnimName))
        {
            PlayAnimationByName(FName(*AnimName));
            Result.bSuccess = true;
        }
    }
    
    return Result;
}
```

### 14.4 Event Injection Pattern

Inject events from game systems to trigger AI state transitions:

```cpp
// From detection system
void AMyCharacter::OnEnemyDetected(AActor* Enemy)
{
    if (AIComponent)
    {
        AIComponent->EnqueueSimpleEvent(TEXT("EnemyDetected"));
        AIComponent->SetBlackboardVector(TEXT("LastKnownEnemyPos"), Enemy->GetActorLocation());
    }
}

// From damage system
void AMyCharacter::OnDamageReceived(float Damage, AActor* DamageCauser)
{
    if (AIComponent)
    {
        AIComponent->EnqueueSimpleEvent(TEXT("TookDamage"));
        if (DamageCauser)
        {
            AIComponent->SetBlackboardVector(TEXT("DamageSourcePos"), DamageCauser->GetActorLocation());
        }
    }
}

// From dialogue system
void AMyCharacter::OnDialogueComplete(FName DialogueId)
{
    if (AIComponent)
    {
        AIComponent->EnqueueSimpleEvent(TEXT("DialogueEnd"));
    }
}

// From interaction system
void AMyCharacter::OnPlayerInteract(AActor* Player)
{
    if (AIComponent)
    {
        AIComponent->EnqueueSimpleEvent(TEXT("PlayerInteract"));
        AIComponent->SetBlackboardVector(TEXT("PlayerPosition"), Player->GetActorLocation());
    }
}
```

### 14.5 Common Blackboard Keys Convention

Recommended naming conventions for blackboard keys:

| Category | Key Pattern | Type | Example Use |
|----------|-------------|------|-------------|
| Health | `Health`, `HealthPercent`, `IsLowHealth` | Float, Float, Bool | Flee when `HealthPercent < 25` |
| Combat | `HasWeapon`, `AmmoCount`, `IsReloading` | Bool, Int, Bool | Reload when `AmmoCount == 0` |
| Movement | `IsMoving`, `InCover`, `CanSprint` | Bool, Bool, Bool | Use cover in combat |
| Detection | `EnemyVisible`, `EnemyCount`, `ThreatLevel` | Bool, Int, Float | Retreat when `EnemyCount > 2` |
| Targets | `*Position` (EnemyPosition, HomePosition) | Vector | Used by MoveTo action |
| State | `Is*` (IsAlerted, IsFleeing, IsDead) | Bool | High-level state flags |

### 14.6 Performance Optimization

```cpp
// Adjust tick rate based on importance
void AMyCharacter::OptimizeAITick()
{
    if (!AIComponent) return;
    
    float DistToPlayer = FVector::Dist(GetActorLocation(), GetPlayerLocation());
    
    if (DistToPlayer > 5000.0f)
    {
        AIComponent->SetTickInterval(1.0f);  // Very far: tick once/second
    }
    else if (DistToPlayer > 2000.0f)
    {
        AIComponent->SetTickInterval(0.2f);  // Far: tick 5x/second
    }
    else
    {
        AIComponent->SetTickInterval(0.0f);  // Near: tick every frame
    }
}

// Batch blackboard updates
void AMyCharacter::SyncBlackboardBatched()
{
    if (!AIComponent || !ShouldSyncThisFrame()) return;
    
    // Only sync when values actually change
    float NewHealth = GetCurrentHealth();
    if (FMath::Abs(LastSyncedHealth - NewHealth) > 1.0f)
    {
        AIComponent->SetBlackboardFloat(TEXT("Health"), NewHealth);
        LastSyncedHealth = NewHealth;
    }
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
