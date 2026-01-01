# P_EAIS Developer Guide

This guide covers the implementation details, extension points, and workflow for the **Enhanced AI System**.

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [JSON AI Schema](#2-json-ai-schema)
3. [Core Components](#3-core-components)
4. [Creating Custom Actions](#4-creating-custom-actions)
5. [Adding Condition Providers](#5-adding-condition-providers)
6. [Blackboard System](#6-blackboard-system)
7. [Visual AI Editor](#7-visual-ai-editor)
8. [P_MEIS Integration](#8-p_meis-integration)
9. [Debugging & Profiling](#9-debugging--profiling)
10. [CI/Automation](#10-ciautomation)
11. [Best Practices](#11-best-practices)

---

## 1. Architecture Overview

### Component Hierarchy

```
UEAISSubsystem (GameInstanceSubsystem)
    ├── Action Registry (TMap<FString, UAIAction*>)
    └── Global Settings

UAIComponent (ActorComponent, attached to Pawn/Controller)
    ├── FAIInterpreter (State Machine Runtime)
    │   ├── FAIBehaviorDef (Parsed JSON)
    │   ├── Blackboard (TMap<FString, FBlackboardValue>)
    │   └── Event Queue (TArray<FAIQueuedEvent>)
    └── Delegates (OnStateChanged, OnActionExecuted)

UAIBehaviour (PrimaryDataAsset)
    ├── EmbeddedJson or JsonFilePath
    └── FAIBehaviorDef (Cached parse result)
```

### Execution Flow

1. **Initialization:** `UAIComponent::BeginPlay()` loads and parses the AI behavior
2. **Start:** `UAIComponent::StartAI()` resets the interpreter and enters the initial state
3. **Tick:** `FAIInterpreter::Tick()` is called each frame (or at TickInterval):
   - Process queued events
   - Execute OnTick actions for current state
   - Evaluate transitions
   - If condition met, exit current state, enter new state
4. **Events:** External systems call `EnqueueEvent()` to trigger transitions

### Network Authority

By default, AI runs on the **server** only (`EAIRunMode::Server`). This ensures deterministic behavior in multiplayer. Cosmetic client-only AI can use `EAIRunMode::Client`.

---

## 2. JSON AI Schema

### Core Concepts

| Concept | Description |
|---------|-------------|
| **State** | A discrete mode of behavior with entry/tick/exit actions |
| **Transition** | A rule to switch states when a condition is met |
| **Condition** | An expression that evaluates to true/false |
| **Action** | A command executed by the AI (MoveTo, Kick, etc.) |
| **Blackboard** | Per-instance key-value memory |

### Schema Format

```json
{
  "name": "BehaviorName",
  "blackboard": {
    "Key1": true,
    "Key2": 0.5,
    "Key3": "string"
  },
  "states": {
    "StateA": {
      "OnEnter": [{ "Action": "ActionName", "Params": {} }],
      "OnTick": [{ "Action": "ActionName", "Params": {} }],
      "OnExit": [{ "Action": "ActionName", "Params": {} }],
      "Transitions": [
        { "Target": "StateB", "Condition": "..." }
      ]
    }
  }
}
```

### Condition Types

| Type | Description | Example |
|------|-------------|---------|
| `Blackboard` | Check blackboard key | `{"type":"Blackboard","key":"HasBall","op":"==","value":true}` |
| `Event` | Check if event occurred | `{"type":"Event","name":"BallSeen"}` |
| `Timer` | Check elapsed time in state | `{"type":"Timer","seconds":2.0}` |
| `Distance` | Check distance to target | `{"type":"Distance","target":"ball","op":"<","value":"500"}` |
| `Custom` | C++ registered condition | `{"type":"Custom","name":"MyCondition"}` |

### Operators

`==`, `!=`, `>`, `<`, `>=`, `<=`

### Step-by-Step: Writing Your First AI Profile

Follow this guide to create a custom AI behavior from scratch.

#### Step 1: Create the JSON File

Create a new file in `Content/AIProfiles/` (e.g., `MyCustomAI.json`):

```json
{
  "name": "MyCustomAI",
  "blackboard": {},
  "states": {}
}
```

#### Step 2: Define Initial Blackboard Values

The blackboard stores AI memory. Define default values:

```json
{
  "name": "MyCustomAI",
  "blackboard": {
    "HasBall": false,
    "NearGoal": false,
    "Tired": false
  },
  "states": {}
}
```

#### Step 3: Create the First State (Idle)

Every AI needs a starting state. The first state in `states` becomes the initial state:

```json
"states": {
  "Idle": {
    "OnEnter": [
      { "Action": "LookAround" }
    ],
    "OnTick": [],
    "Transitions": [
      { "Target": "ChaseBall", "Condition": { "type": "Event", "name": "BallSeen" } }
    ]
  }
}
```

#### Step 4: Add More States

Build out the behavior with additional states:

```json
"ChaseBall": {
  "OnTick": [
    { "Action": "MoveTo", "Params": { "Target": "ball" } }
  ],
  "Transitions": [
    { "Target": "AttackGoal", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
  ]
},
"AttackGoal": {
  "OnEnter": [
    { "Action": "AimAt", "Params": { "Target": "opponentGoal" } }
  ],
  "OnTick": [
    { "Action": "MoveTo", "Params": { "Target": "opponentGoal", "Speed": 1.0 } }
  ],
  "Transitions": [
    { "Target": "Shoot", "Condition": { "type": "Distance", "target": "opponentGoal", "op": "<", "value": 800 } },
    { "Target": "ChaseBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": false } }
  ]
},
"Shoot": {
  "OnEnter": [
    { "Action": "Kick", "Params": { "Power": 0.95 } }
  ],
  "Transitions": [
    { "Target": "Idle", "Condition": { "type": "Timer", "seconds": 0.5 } }
  ]
}
```

#### Step 5: Complete Example

Here's a complete, functional AI profile:

```json
{
  "name": "CustomStriker",
  "blackboard": {
    "HasBall": false,
    "shotCooldown": 0.0
  },
  "states": {
    "Idle": {
      "OnEnter": [{ "Action": "LookAround" }],
      "OnTick": [],
      "Transitions": [
        { "Target": "ChaseBall", "Condition": { "type": "Event", "name": "BallSeen" } },
        { "Target": "ChaseBall", "Condition": { "type": "Timer", "seconds": 2.0 } }
      ]
    },
    "ChaseBall": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ball" } }
      ],
      "Transitions": [
        { "Target": "AttackGoal", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
      ]
    },
    "AttackGoal": {
      "OnEnter": [
        { "Action": "AimAt", "Params": { "Target": "opponentGoal" } }
      ],
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "opponentGoal", "Speed": 1.0 } }
      ],
      "Transitions": [
        { "Target": "Shoot", "Condition": { "type": "Distance", "target": "opponentGoal", "op": "<", "value": 800 } },
        { "Target": "ChaseBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": false } }
      ]
    },
    "Shoot": {
      "OnEnter": [
        { "Action": "Kick", "Params": { "Power": 0.9 } }
      ],
      "Transitions": [
        { "Target": "Idle", "Condition": { "type": "Timer", "seconds": 0.5 } }
      ]
    }
  }
}
```

#### Step 6: Test Your Profile

1. Save the file as `Content/AIProfiles/CustomStriker.json`
2. Open the game and run:
```cpp
EAIS.SpawnBot 1 CustomStriker
EAIS.Debug 1
```
3. Watch the AI state changes above the character

### Available Actions Reference

| Action | Description | Parameters |
|--------|-------------|------------|
| `MoveTo` | Navigate to target | `Target`: "ball", "opponentGoal", "ownGoal", "home" <br> `Speed`: 0.0-1.0 (optional) |
| `Kick` | Kick the ball | `Power`: 0.0-1.0 |
| `AimAt` | Set look direction | `Target`: "ball", "opponentGoal", actor name |
| `SetLookTarget` | Focus on target | `Target`: actor reference |
| `Wait` | Do nothing for duration | `Power`: seconds |
| `SetBlackboardKey` | Update blackboard | `Target`: key name, `value`: any |
| `InjectInput` | Press a button via P_MEIS | `Target`: action name (e.g., "Jump", "Sprint") |
| `PassToTeammate` | Pass to nearest teammate | `Power`: 0.0-1.0 |
| `LookAround` | Clear focus target | (none) |

### Blackboard Keys (Auto-Synced by MF_AICharacter)

These keys are automatically updated from game state:

| Key | Type | Description |
|-----|------|-------------|
| `HasBall` | Bool | Does this character have the ball? |
| `Ball` | Object | Reference to the ball actor |
| `BallPosition` | Vector | World location of the ball |
| `TeamID` | Float | Team enum as float (0=None, 1=TeamA, 2=TeamB) |
| `IsStunned` | Bool | Is character currently stunned? |
| `IsSprinting` | Bool | Is character sprinting? |
| `MyPosition` | Vector | Character's world location |
| `Controller` | Object | Current controller reference |

### Common Patterns

#### Patrol Behavior
```json
"Patrol": {
  "OnTick": [
    { "Action": "MoveTo", "Params": { "Target": "home", "Speed": 0.5 } }
  ],
  "Transitions": [
    { "Target": "ChaseBall", "Condition": { "type": "Distance", "target": "ball", "op": "<", "value": 1000 } }
  ]
}
```

#### Defensive Positioning
```json
"DefendGoal": {
  "OnTick": [
    { "Action": "MoveTo", "Params": { "Target": "ownGoal", "Speed": 0.7 } }
  ],
  "Transitions": [
    { "Target": "Intercept", "Condition": { "type": "Distance", "target": "ball", "op": "<", "value": 500 } }
  ]
}
```

#### Cooldown After Action
```json
"AfterShot": {
  "OnEnter": [
    { "Action": "SetBlackboardKey", "Params": { "Target": "shotCooldown", "value": 3.0 } }
  ],
  "Transitions": [
    { "Target": "Idle", "Condition": { "type": "Timer", "seconds": 1.0 } }
  ]
}
```

---

## 3. Core Components

### UEAISSubsystem

Global subsystem managing the action registry and settings.

```cpp
// Get subsystem
UEAISSubsystem* Subsystem = UEAISSubsystem::Get(WorldContextObject);

// Register custom action
Subsystem->RegisterAction(TEXT("MyAction"), UMyAction::StaticClass());

// Get action instance
UAIAction* Action = Subsystem->GetAction(TEXT("MoveTo"));
```

### UAIBehaviour

Primary asset type for storing AI behavior definitions.

```cpp
// Create programmatically
UAIBehaviour* Behavior = NewObject<UAIBehaviour>();
Behavior->EmbeddedJson = JsonString;
Behavior->ParseBehavior(Error);

// Or reference external file
Behavior->JsonFilePath = TEXT("Striker.json");
```

### UAIComponent

Attach to a Pawn or Controller to give it AI.

```cpp
// Add to pawn
UAIComponent* AIComp = NewObject<UAIComponent>(MyPawn);
AIComp->RegisterComponent();
AIComp->JsonFilePath = TEXT("Striker.json");
AIComp->bAutoStart = true;

// Control
AIComp->StartAI();
AIComp->StopAI();
AIComp->ResetAI();

// Events
AIComp->EnqueueSimpleEvent(TEXT("BallSeen"));

// Blackboard
AIComp->SetBlackboardBool(TEXT("HasBall"), true);
bool HasBall = AIComp->GetBlackboardBool(TEXT("HasBall"));
```

### FAIInterpreter

The runtime state machine. Usually accessed through UAIComponent.

---

## 4. Creating Custom Actions

### Step 1: Define the Action Class

```cpp
UCLASS()
class UAIAction_CustomKick : public UAIAction
{
    GENERATED_BODY()
    
public:
    virtual void Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params) override
    {
        APawn* Pawn = OwnerComponent->GetOwnerPawn();
        if (!Pawn) return;
        
        // Access parameters
        float Power = Params.Power;
        FString Target = Params.Target;
        
        // Your custom logic
        UE_LOG(LogTemp, Log, TEXT("Custom kick with power %f"), Power);
        
        // Optionally inject input via P_MEIS
        APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
        if (PC)
        {
            UCPP_BPL_InputBinding::InjectActionTriggered(PC, FName(TEXT("Kick")));
        }
    }
    
    virtual FString GetActionName() const override { return TEXT("CustomKick"); }
};
```

### Step 2: Register the Action

In your game module or subsystem initialization:

```cpp
UEAISSubsystem* Subsystem = UEAISSubsystem::Get(GetWorld());
Subsystem->RegisterAction(TEXT("CustomKick"), UAIAction_CustomKick::StaticClass());
```

### Step 3: Use in JSON

```json
{
  "Action": "CustomKick",
  "Params": { "Power": 0.8 }
}
```

---

## 5. Adding Condition Providers

Custom conditions allow you to extend the condition system beyond built-in types.

```cpp
// TODO: Implement condition provider system
// This is a planned feature for a future update

// Current workaround: Use Blackboard conditions
// Update blackboard values from external systems, then check them in transitions
```

---

## 6. Blackboard System

The blackboard is per-AI-instance memory supporting multiple types.

### Supported Types

| Type | Get/Set Methods |
|------|-----------------|
| Bool | `GetBlackboardBool()`, `SetBlackboardBool()` |
| Int | via `FBlackboardValue` |
| Float | `GetBlackboardFloat()`, `SetBlackboardFloat()` |
| String | via `FBlackboardValue` |
| Vector | `GetBlackboardVector()`, `SetBlackboardVector()` |
| Object | `GetBlackboardObject()`, `SetBlackboardObject()` |

### Setting from External Systems

```cpp
// Example: Ball possession system updates AI blackboard
void ABall::OnPossessionChanged(APawn* NewOwner)
{
    // Update all AI components
    TArray<UAIComponent*> AIComponents;
    // ... find components ...
    
    for (UAIComponent* AI : AIComponents)
    {
        bool bHasBall = (AI->GetOwnerPawn() == NewOwner);
        AI->SetBlackboardBool(TEXT("HasBall"), bHasBall);
        AI->SetBlackboardObject(TEXT("BallOwner"), NewOwner);
    }
}
```

---

## 7. Visual AI Editor

The Visual AI Editor is a graphical tool for creating, editing, validating, and testing AI behavior profiles.

### Opening the Editor

**From Menu:** Tools → EAIS → EAIS AI Editor

This opens a dockable Slate-based editor window (similar to MWCS).

### Editor Interface

```
┌─────────────────────────────────────────────────┐
│  EAIS — Enhanced AI System                      │
├─────────────────────────────────────────────────┤
│ [Load Profile] [Save Profile] [Validate] [Format]│
│ [List Profiles] [Generate/Repair Editor EUW]     │
├─────────────────────────────────────────────────┤
│  EAIS Tool ready.                                │
│  > [Log output appears here]                     │
│                                                  │
└─────────────────────────────────────────────────┘
```

### Editor Features

| Feature | Button | Description |
|---------|--------|-------------|
| **List Profiles** | `List Profiles` | Show all available AI profiles |
| **Load** | `Load Profile` | Load a profile (enter name in console) |
| **Save** | `Save Profile` | Save current profile to file |
| **Validate** | `Validate` | Check JSON for errors |
| **Format** | `Format` | Auto-format/prettify JSON |
| **Generate EUW** | `Generate/Repair Editor EUW` | Generate Editor Utility Widget via P_MWCS |

### Step-by-Step: Creating an AI Profile in the Editor

#### Step 1: Open the Editor
```
Tools → EAIS → EAIS AI Editor
```

#### Step 2: Create New Profile
Click **New** to get a starter template:

```json
{
  "name": "NewProfile",
  "blackboard": {
    "HasBall": false
  },
  "states": {
    "Idle": {
      "OnEnter": [{ "Action": "LookAround" }],
      "OnTick": [],
      "Transitions": [
        { "Target": "ChaseBall", "Condition": { "type": "Event", "name": "BallSeen" } }
      ]
    },
    "ChaseBall": {
      "OnTick": [{ "Action": "MoveTo", "Params": { "Target": "ball" } }],
      "Transitions": [
        { "Target": "Idle", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
      ]
    }
  }
}
```

#### Step 3: Edit the JSON
Modify the JSON directly in the editor. Add states, actions, and transitions.

#### Step 4: Validate
Click **Validate** to check for errors:
- ✓ Valid = Green checkmark, ready to save
- ✕ Error = Red X with error message

#### Step 5: Save
1. Enter a profile name (e.g., "MyCustomAI")
2. Click **Save**
3. File is saved to `Content/AIProfiles/MyCustomAI.json`

#### Step 6: Test
Click **Test Spawn AI** to spawn an AI character using your profile.

### Example: Creating a Defender AI

```json
{
  "name": "Defender",
  "blackboard": {
    "HasBall": false,
    "EnemyNear": false,
    "HomePosition": [0, 0, 0]
  },
  "states": {
    "Guard": {
      "OnEnter": [
        { "Action": "SetLookTarget", "Params": { "Target": "ball" } }
      ],
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ownGoal", "Speed": 0.4 } }
      ],
      "Transitions": [
        { "Target": "Intercept", "Condition": { "type": "Distance", "target": "ball", "op": "<", "value": 800 } },
        { "Target": "ChaseBall", "Condition": { "type": "Blackboard", "key": "EnemyNear", "op": "==", "value": true } }
      ]
    },
    "Intercept": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ball", "Speed": 1.0 } }
      ],
      "Transitions": [
        { "Target": "ClearBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } },
        { "Target": "Guard", "Condition": { "type": "Distance", "target": "ball", "op": ">", "value": 1500 } }
      ]
    },
    "ChaseBall": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ball" } }
      ],
      "Transitions": [
        { "Target": "ClearBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
      ]
    },
    "ClearBall": {
      "OnEnter": [
        { "Action": "AimAt", "Params": { "Target": "opponentGoal" } },
        { "Action": "Kick", "Params": { "Power": 0.8 } }
      ],
      "Transitions": [
        { "Target": "Guard", "Condition": { "type": "Timer", "seconds": 0.5 } }
      ]
    }
  }
}
```

### Example: Creating a Goalkeeper AI

```json
{
  "name": "Goalkeeper",
  "blackboard": {
    "HasBall": false,
    "BallDangerous": false
  },
  "states": {
    "Position": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ownGoal", "Speed": 0.3 } },
        { "Action": "SetLookTarget", "Params": { "Target": "ball" } }
      ],
      "Transitions": [
        { "Target": "Dive", "Condition": { "type": "Distance", "target": "ball", "op": "<", "value": 400 } },
        { "Target": "Rush", "Condition": { "type": "Blackboard", "key": "BallDangerous", "op": "==", "value": true } }
      ]
    },
    "Dive": {
      "OnEnter": [
        { "Action": "MoveTo", "Params": { "Target": "ball", "Speed": 1.0 } }
      ],
      "Transitions": [
        { "Target": "ClearBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } },
        { "Target": "Position", "Condition": { "type": "Timer", "seconds": 1.5 } }
      ]
    },
    "Rush": {
      "OnTick": [
        { "Action": "MoveTo", "Params": { "Target": "ball", "Speed": 0.9 } }
      ],
      "Transitions": [
        { "Target": "ClearBall", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } },
        { "Target": "Position", "Condition": { "type": "Distance", "target": "ball", "op": ">", "value": 600 } }
      ]
    },
    "ClearBall": {
      "OnEnter": [
        { "Action": "AimAt", "Params": { "Target": "teammate" } },
        { "Action": "Kick", "Params": { "Power": 0.7 } }
      ],
      "Transitions": [
        { "Target": "Position", "Condition": { "type": "Timer", "seconds": 0.5 } }
      ]
    }
  }
}
```

### Validation Errors

| Error | Cause | Fix |
|-------|-------|-----|
| "Invalid JSON syntax" | Malformed JSON | Check brackets, commas, quotes |
| "Missing required field: 'name'" | No `name` field | Add `"name": "ProfileName"` |
| "Missing required field: 'states'" | No `states` object | Add `"states": { ... }` |
| "State 'X' must be an object" | State is not an object | Ensure state is `{ }` not string |

### Keyboard Shortcuts (In JSON Editor)

| Key | Action |
|-----|--------|
| `Ctrl+S` | Save profile |
| `Ctrl+Shift+F` | Format JSON |
| `F5` | Validate |

### Regenerating the Editor Widget

To regenerate or repair the Editor Utility Widget:

**Option 1: From EAIS Tool**
```
Tools → EAIS → EAIS AI Editor → Click "Generate/Repair Editor EUW"
```

**Option 2: Run MWCS commandlet**
```powershell
.\DevTools\scripts\generate_Editor.bat
```

This runs P_MWCS to create the Editor Utility Widget at `Content/Editor/EAIS/EUW_EAIS_AIEditor`.

---

## 8. P_MEIS Integration

### Philosophy

AI should play "fair" - using the same input system as players. This:
- Ensures balanced gameplay
- Simplifies input handling code
- Makes AI behavior transparent

### Input Injection

```cpp
// In your action
APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
if (PC)
{
    // Press: Started
    UCPP_BPL_InputBinding::InjectActionStarted(PC, FName(TEXT("Jump")));
    
    // Hold: Triggered
    UCPP_BPL_InputBinding::InjectActionTriggered(PC, FName(TEXT("Sprint")));
    
    // Release: Completed
    UCPP_BPL_InputBinding::InjectActionCompleted(PC, FName(TEXT("Jump")));
    
    // Axis (joystick)
    UCPP_BPL_InputBinding::InjectAxis2D(PC, FName(TEXT("Move")), FVector2D(1, 0));
}
```

### Input Mapping

Configure in `Config/DefaultEAIS.ini`:

```ini
[EAIS.InputMapping]
+InputToEvent=(InputAction="Kick", EventName="KickCommand")
```

---

## 9. Debugging & Profiling

### Debug Mode

```cpp
// Enable per-component
AIComponent->bDebugMode = true;

// Enable globally
EAIS.Debug 1
```

### Console Commands

| Command | Description |
|---------|-------------|
| `EAIS.Debug 1` | Enable debug logging |
| `EAIS.ListActions` | Show registered actions |
| `EAIS.InjectEvent * BallSeen` | Inject event to all AI |

### Debug Overlay

When debug is enabled, the AI shows:
- Current state name
- Blackboard values
- Recent transitions

### Profiling

Use Unreal Insights or `stat P_EAIS` (if implemented) to profile AI tick costs.

**Tips:**
- Increase `TickInterval` to reduce CPU cost
- Stagger AI ticks across frames
- Keep actions lightweight; use async for long operations

---

## 10. CI/Automation

### Scripts

| Script | Purpose |
|--------|---------|
| `build_headless.bat/.sh` | Compile plugin |
| `run_tests.bat/.sh` | Run automation tests |
| `generate_Editor.bat` | Regenerate Editor via P_MWCS |

### GitHub Actions

See `DevTools/ci/eais_ci.yml` for CI workflow. Requires:
- Self-hosted runner with UE installed
- `UE_ROOT` environment variable set

### Automation Tests

Test files in `Content/AIProfiles/Tests/`:
- `Test_SimpleState.json` - State transition test
- `Test_EventTransition.json` - Event handling test

---

## 11. In-Level Testing Instructions

This section explains how to test AI behaviors directly in the Unreal Editor.

### Step 1: Open a Test Level

1. Open `Content/Maps/L_MiniFootball` (or any gameplay map)
2. Ensure the level has:
   - A `NavMeshBoundsVolume` covering the playable area
   - Goal actors for both teams
   - A ball spawn point

### Step 2: Spawn AI Bots

Use the console command (press `~` to open console):

```cpp
// Spawn a Striker on Team A (TeamID 1)
EAIS.SpawnBot 1 Striker

// Spawn a Defender on Team B (TeamID 2)
EAIS.SpawnBot 2 Defender

// Spawn a Goalkeeper on Team A
EAIS.SpawnBot 1 Goalkeeper
```

### Step 3: Enable Debug Mode

```cpp
// Turn on debug visualization for all AI
EAIS.Debug 1

// This shows:
// - Current state name above each AI
// - Blackboard values
// - Transition history
```

### Step 4: Inject Events

Test event-driven transitions:

```cpp
// Send event to all AI (use * as wildcard)
EAIS.InjectEvent * BallSeen

// Send event to specific AI by name
EAIS.InjectEvent Striker_0 GotBall
```

### Step 5: Observe Behavior

Watch the AI:
- **Striker**: Should chase ball, move toward goal, and shoot when close
- **Defender**: Should patrol, intercept, and tackle
- **Goalkeeper**: Should stay near goal and block shots

### Step 6: Validate Actions

Check that actions are executing:

```cpp
// List all registered actions
EAIS.ListActions

// Output Log will show action execution details when Debug is on
```

### Manual Verification Checklist

- [ ] AI spawns at correct team spawn point
- [ ] AI moves toward ball when appropriate
- [ ] AI transitions between states correctly
- [ ] AI uses P_MEIS input injection (not direct API calls)
- [ ] AI respects team boundaries
- [ ] Blackboard values update correctly
- [ ] No crashes or infinite loops

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| AI doesn't move | Missing NavMesh | Add `NavMeshBoundsVolume` and rebuild |
| AI stuck in state | Missing transition condition | Check JSON for transition rules |
| AI ignores ball | Blackboard not synced | Verify `AMF_AICharacter::SyncBlackboard()` |
| Actions not firing | Action not registered | Check `EAIS.ListActions` output |

---

## 12. Best Practices

### JSON Authoring

1. **Use descriptive state names:** `ChaseBall`, not `State1`
2. **Keep states focused:** One responsibility per state
3. **Use priorities:** For complex transition logic
4. **Document with comments:** JSON doesn't support comments, use a README

### Performance

1. **Avoid per-tick allocations:** Pool objects where possible
2. **Use TickInterval > 0:** Not every AI needs 60 ticks/second
3. **Stagger initialization:** Don't spawn 50 AI in one frame
4. **Keep blackboard small:** Only store what's needed

### Multiplayer

1. **Run on server:** `EAIRunMode::Server` for authoritative AI
2. **Replicate selectively:** Only blackboard keys clients need
3. **Predict locally:** Consider client prediction for responsiveness

### Debugging

1. **Enable debug mode:** During development
2. **Use test profiles:** Deterministic behaviors for testing
3. **Log transitions:** Helps diagnose unexpected behavior

---

## Appendix: Class Reference

| Class | Description |
|-------|-------------|
| `UEAISSubsystem` | Global subsystem for actions and settings |
| `UAIBehaviour` | Data asset for behavior definitions |
| `UAIComponent` | Component adding AI to actors |
| `FAIInterpreter` | State machine runtime |
| `UAIAction` | Base class for actions |
| `EAIS_Types.h` | Core structs and enums |

---

*Last updated: December 2025*
