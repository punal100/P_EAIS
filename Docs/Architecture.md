# P_EAIS Architecture

## Overview

P_EAIS (Enhanced AI System) is a modular AI runtime for Unreal Engine that implements a **Deterministic, Server-Authoritative Hybrid FSM/BT Runtime**.

## Core Principles

### 1. Server Authority
- EAIS ticks ONLY on the Server
- No AI decisions on Client
- AI never sends RPCs directly

### 2. Intent Injection
- EAIS NEVER directly modifies gameplay state
- EAIS injects intent via actions
- Optional P_MEIS integration for input injection

### 3. Determinism
- State transitions are deterministic
- Single-transition-per-tick
- Event order is deterministic (FIFO)
- Replay-safe

## Module Structure

```
P_EAIS/
├── Source/
│   ├── P_EAIS/           # Runtime module
│   │   ├── Public/
│   │   │   ├── EAIS_Types.h          # Canonical types
│   │   │   ├── AIInterpreter.h       # FSM interpreter
│   │   │   ├── AIComponent.h         # UActorComponent
│   │   │   ├── AIAction.h            # Action base class
│   │   │   └── EAISSubsystem.h       # Game instance subsystem
│   │   └── Private/
│   │
│   ├── P_EAIS_Editor/    # Editor-only module
│   │   ├── Public/
│   │   │   ├── UEAIS_GraphNode.h
│   │   │   ├── EAIS_GraphSchema.h
│   │   │   └── SEAIS_GraphEditor.h
│   │   └── Private/
│   │
│   └── P_EAISTools/      # Editor tools
│
├── Content/AIProfiles/   # Runtime JSON profiles
├── Editor/AI/            # Editor JSON (with layout)
└── DevTools/             # CI, output, and scripts
    ├── ci/               # CI configuration
    ├── output/           # Build/test output
    └── scripts/          # Validation scripts
```

## Data Flow

```
┌─────────────────┐      ┌─────────────────┐
│   JSON Profile  │─────>│   AIBehaviour   │
│ (.runtime.json) │      │   (UObject)     │
└─────────────────┘      └────────┬────────┘
                                  │
                                  v
┌─────────────────┐      ┌─────────────────┐
│   AIComponent   │<─────│  FAIInterpreter │
│ (UActorComponent)│      │   (State FSM)   │
└────────┬────────┘      └────────┬────────┘
         │                        │
         │ Tick()                 │ EnqueueEvent()
         v                        v
┌─────────────────┐      ┌─────────────────┐
│  Execute Actions│─────>│   Game Systems  │
│  (UAIAction)    │      │   (via intent)  │
└─────────────────┘      └─────────────────┘
```

## Key Classes

### FAIInterpreter
The core FSM interpreter:
- State transitions
- Event queue processing
- Blackboard management
- Action execution

### UAIComponent
ActorComponent that drives AI:
- Holds FAIInterpreter instance
- Ticks the interpreter
- Provides Blueprint interface

### UAIAction
Base class for actions:
- `Execute()` - Perform the action
- `Abort()` - Cancel latent action
- `IsLatent()` - Whether action runs over time

## Tick Order

1. **AIComponent::TickComponent()**
2. **FAIInterpreter::Tick()**
   - Update timers
   - Process queued events
   - Execute OnTick actions
   - Evaluate transitions (priority-sorted)
   - Clear recent events

## Optional Dependencies

- **P_MEIS**: Enhanced Input System (for input injection)
- **P_MWCS**: Widget Creation System (for Editor Utility Widget)

## Threading Model

- All EAIS code runs on Game Thread
- No async operations in interpreter
- Actions may use async (but must handle abort)

---

*P_EAIS - Modular AI System*
