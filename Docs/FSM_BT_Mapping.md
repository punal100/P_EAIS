# EAIS FSM → Behavior Tree Mapping

## Overview

EAIS is NOT a traditional Behavior Tree. It is a **State-based BT hybrid** that combines the simplicity of Finite State Machines with the structure of Behavior Trees.

## Concept Mapping

| Behavior Tree Concept | EAIS Equivalent |
|----------------------|-----------------|
| **Selector** | State with prioritized transitions |
| **Sequence** | Ordered OnEnter / OnTick actions |
| **Decorator** | Transition Condition |
| **Leaf Task** | UAIAction |
| **Blackboard** | EAIS Blackboard |
| **Event** | EAIS Event Queue |

## What EAIS Intentionally Avoids

### 1. Parallel Nodes
EAIS does not support parallel execution. Only one state is active at a time.

**Rationale**: Parallel nodes create ambiguity in:
- State ownership
- Transition priority
- Action conflicts

### 2. Latent Task Blocking
Traditional BTs often "block" on latent tasks. EAIS actions are either:
- **Instant**: Execute and return immediately
- **Latent**: Start execution, check completion via condition

**Rationale**: Blocking creates non-deterministic timing.

### 3. Tick Ownership Ambiguity
In traditional BTs, "who ticks?" is complex. In EAIS:
- AIComponent ticks the Interpreter
- Interpreter ticks all active actions
- Clear ownership chain

## State Machine Semantics

### States
Each state has:
- **OnEnter**: Actions executed once when entering
- **OnTick**: Actions executed every tick while in state
- **OnExit**: Actions executed once when leaving
- **Transitions**: Conditions to move to other states

### Transitions
Transitions are evaluated:
1. After OnTick actions
2. In priority order (highest first)
3. First match wins (single transition per tick)

### Events
Events are:
1. Queued during gameplay
2. Processed at start of tick
3. Consumed once
4. Not persistent across states

## Example Mapping

### Traditional Behavior Tree
```
Root (Selector)
├─ HasTarget (Decorator)
│  └─ Attack (Sequence)
│     ├─ MoveToTarget
│     └─ ExecuteAttack
└─ NoTarget (Decorator)
   └─ Patrol (Task)
```

### EAIS Equivalent
```json
{
  "states": [
    {
      "id": "Idle",
      "terminal": false,
      "onTick": [{ "actionName": "LookAround" }],
      "transitions": [
        {
          "to": "Attack",
          "priority": 200,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "HasTarget",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        },
        {
          "to": "Patrol",
          "priority": 100,
          "condition": {
            "type": "Timer",
            "seconds": 2.0
          }
        }
      ]
    },
    {
      "id": "Patrol",
      "terminal": false,
      "onTick": [{ "actionName": "MoveToPatrolPoint" }],
      "transitions": [
        {
          "to": "Attack",
          "priority": 100,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "HasTarget",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "true" }
          }
        }
      ]
    },
    {
      "id": "Attack",
      "terminal": false,
      "onTick": [
        { "actionName": "MoveToTarget" },
        { "actionName": "ExecuteAttack" }
      ],
      "transitions": [
        {
          "to": "Idle",
          "priority": 100,
          "condition": {
            "type": "Blackboard",
            "keyOrName": "HasTarget",
            "op": "Equal",
            "compareValue": { "type": "Bool", "rawValue": "false" }
          }
        }
      ]
    }
  ]
}
```

## Design Benefits

1. **Simpler Mental Model**: States are easier to visualize than nested trees
2. **Explicit Priorities**: No ambiguity in evaluation order
3. **Deterministic**: Same inputs always produce same outputs
4. **JSON-Friendly**: Easy to serialize, version, and diff
5. **Debuggable**: Clear state transitions in debug overlay

---

*P_EAIS - Modular AI System*
