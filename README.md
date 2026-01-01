# P_EAIS - Enhanced AI System

**P_EAIS** is a modular AI plugin for Unreal Engine 5 that provides a **JSON-programmable AI runtime**, a **Visual AI Editor**, and deep integration with **P_MEIS** (Input) and **P_MWCS** (UI).

It brings the "Old RPG" style of transparent, state-machine driven AI to modern Unreal Engine projects, with a focus on fairness (AI uses the same Input API as players via P_MEIS injection).

## 🌟 Key Features

- **JSON-First Architecture:** Define AI brains in human-readable JSON files. No Blueprint spaghetti required.
- **State Machine Runtime:** Hierarchical states with transitions, conditions, and actions.
- **Blackboard System:** Per-instance key-value storage for AI memory.
- **Input Injection:** AI agents "press buttons" via `P_MEIS` Injection, ensuring they play by the same rules as humans.
- **Visual AI Editor:** Editor Utility Widget for creating, editing, validating, and testing AI behaviors.
- **Headless Automation:** Includes scripts for headless building, testing, and CI integration.
- **MiniFootball Ready:** Comes with `Striker`, `Defender`, and `Goalkeeper` AI profiles.

## 📦 Architecture

```
P_MEIS (Input) -> P_EAIS (Decision / JSON interpreter) -> Pawn/Controller -> P_MiniFootball Gameplay
                                 ^
                                 |
                            AI Editor (UMG) ---- P_MWCS (Widget creation)
```

## 🚀 Quick Start

### 1. Enable Plugins

Ensure `P_EAIS`, `P_MEIS`, and `P_MWCS` are enabled in your project:
- Edit → Plugins → Search "EAIS"

### 2. Spawn AI

```cpp
// Console Command
EAIS.SpawnBot <TeamID> <ProfileName>

// Example: Spawn a Striker on Team 1
EAIS.SpawnBot 1 Striker
```

### 3. Add AI to Existing Pawn

```cpp
// In your Pawn or Controller
UAIComponent* AIComp = NewObject<UAIComponent>(this);
AIComp->RegisterComponent();
AIComp->JsonFilePath = TEXT("Striker.json");
AIComp->bAutoStart = true;
```

### 4. Open the Visual AI Editor

```cpp
// Option 1: From Menu
Window → Developer Tools → EAIS AI Editor

// Option 2: Run directly
Run EUW_EAIS_AIEditor in Content/P_EAIS/Editor/
```

### 5. Create AI Profiles (JSON)

Create JSON files in `Content/AIProfiles/`. See [GUIDE.md](GUIDE.md) for step-by-step authoring instructions.

## 📂 Folder Structure

```
P_EAIS/
├── Source/
│   ├── P_EAIS/                  # Runtime module
│   │   ├── Public/              # Headers (EAIS_Types, AIBehaviour, AIInterpreter, etc.)
│   │   └── Private/             # Implementations
│   └── P_EAISTools/             # Editor module (AI Editor)
├── Content/
│   └── Editor/                  # Editor Utility Widgets
├── DevTools/
│   ├── scripts/                 # build_headless.sh/.bat, run_tests.sh/.bat
│   └── ci/                      # GitHub Actions workflow
├── Docs/
│   ├── README.md
│   └── GUIDE.md
├── Config/
│   └── DefaultEAIS.ini          # Plugin configuration
└── P_EAIS.uplugin
```

**Project-Level AI Profiles:** `Content/AIProfiles/`
- `Striker.json` - Offensive AI
- `Defender.json` - Defensive AI  
- `Goalkeeper.json` - Goal protection AI
- `Tests/` - Automation test profiles

### MiniFootball AI Character Integration

In `P_MiniFootball`, all match characters are `AMF_AICharacter` instances:

- **Match Start**: All characters are AI-controlled
- **Human Joins**: AI stops for that character → Human takes control
- **Human Switches (Q)**: Previous character resumes AI automatically
- **Human Leaves**: Character immediately resumes AI

This is achieved via `PossessedBy()` / `UnPossessed()` overrides that stop/start AI based on controller type.

See [P_MiniFootball README](../P_MiniFootball/README.md) for full details.

## 🎨 Visual AI Editor

The Visual AI Editor provides a graphical interface for creating and managing AI behaviors.

### Features

| Feature | Description |
|---------|-------------|
| **Load/Save** | Load existing profiles, save new ones |
| **JSON Editor** | Direct JSON editing with syntax highlighting |
| **Validate** | Check JSON against schema for errors |
| **Format** | Auto-format/prettify JSON |
| **Test Spawn** | Instantly spawn AI with current profile |

### Opening the Editor

1. **From Menu:** Window → Developer Tools → EAIS AI Editor
2. **Direct:** Run `EUW_EAIS_AIEditor` Editor Utility Widget

### Editor Workflow

1. **New Profile:** Click "New" to create a template
2. **Edit:** Modify the JSON in the editor panel
3. **Validate:** Click "Validate" to check for errors
4. **Save:** Enter a name and click "Save"
5. **Test:** Click "Test Spawn AI" to see it in action

## 🧪 Testing

### In-Editor

1. Open any MiniFootball map (e.g., `Maps/L_MiniFootball`)
2. Spawn a bot: `EAIS.SpawnBot 1 Striker`
3. Toggle Debug: `EAIS.Debug 1`
4. Inject events: `EAIS.InjectEvent * BallSeen`

### Console Commands

| Command | Description |
|---------|-------------|
| `EAIS.SpawnBot <Team> <Profile>` | Spawn AI bot |
| `EAIS.Debug <0\|1>` | Toggle debug mode |
| `EAIS.InjectEvent <AIName\|*> <Event>` | Inject event to AI |
| `EAIS.ListActions` | List registered actions |

### Headless (Automated)

```powershell
# Windows
.\DevTools\scripts\run_tests.bat

# Linux/macOS
./DevTools/scripts/run_tests.sh
```

## 🔧 Integration

### P_MEIS (Input)

P_EAIS does not call movement functions directly. Instead, it injects input via P_MEIS:

```cpp
// Instead of: Pawn->Jump();
// AI does:
UCPP_BPL_InputBinding::InjectActionTriggered(PC, FName(TEXT("Jump")));
```

This ensures AI uses the exact same input processing pipeline as players.

### P_MWCS (UI)

The **AI Editor** tool is generated by `P_MWCS` from a JSON specification:

```
Spec → P_MWCS → EUW_EAIS_AIEditor.uasset
```

To regenerate:
```powershell
.\DevTools\scripts\generate_Editor.bat
```

## 📐 JSON AI Schema

AI behaviors are defined in JSON. See `Docs/ai-schema.json` for the full schema.

### Example: Simple Striker

```json
{
  "name": "Striker",
  "blackboard": {
    "HasBall": false,
    "TargetGoal": "Goal_B"
  },
  "states": {
    "Idle": {
      "OnEnter": [{ "Action": "LookAround" }],
      "Transitions": [
        { "Target": "ChaseBall", "Condition": "CanSeeBall" }
      ]
    },
    "ChaseBall": {
      "OnTick": [{ "Action": "MoveTo", "Params": { "Target": "ball" } }],
      "Transitions": [
        { "Target": "Shoot", "Condition": { "type": "Blackboard", "key": "HasBall", "op": "==", "value": true } }
      ]
    },
    "Shoot": {
      "OnEnter": [
        { "Action": "AimAt", "Params": { "Target": "opponentGoal" } },
        { "Action": "Kick", "Params": { "Power": 0.9 } }
      ],
      "Transitions": [
        { "Target": "Idle", "Condition": { "type": "Timer", "seconds": 1.0 } }
      ]
    }
  }
}
```

## 🏗️ Built-in Actions

| Action | Description | Parameters |
|--------|-------------|------------|
| `MoveTo` | Navigate to target | `Target`, `Speed` |
| `Kick` | Kick the ball | `Power` (0-1) |
| `AimAt` | Set look direction | `Target` |
| `SetLookTarget` | Set focus target | `Target` |
| `Wait` | Passive wait | `Power` (seconds) |
| `SetBlackboardKey` | Set blackboard value | `Target` (key), `value` |
| `InjectInput` | Inject P_MEIS input | `Target` (action name) |
| `PassToTeammate` | Pass ball | `Power` |
| `LookAround` | Clear focus | - |

## 🔌 Extending P_EAIS

### Custom Actions

```cpp
UCLASS()
class UMyCustomAction : public UAIAction
{
    GENERATED_BODY()
public:
    virtual void Execute_Implementation(UAIComponent* Owner, const FAIActionParams& Params) override
    {
        // Your logic here
    }
    
    virtual FString GetActionName() const override { return TEXT("MyAction"); }
};

// Register in subsystem
Subsystem->RegisterAction(TEXT("MyAction"), UMyCustomAction::StaticClass());
```

## 🚦 CI Integration

See `DevTools/ci/eais_ci.yml` for a GitHub Actions example. Requires a self-hosted runner with Unreal Engine installed.

## 📄 License

Part of the A_MiniFootball project by Punal Manalan.

## 📚 Further Reading

- [GUIDE.md](GUIDE.md) - Detailed developer guide with step-by-step AI authoring
- [Docs/ai-schema.json](Docs/ai-schema.json) - JSON Schema reference
