# EAIS Failure Modes Documentation

## Overview

This document describes common failure modes in P_EAIS and how to handle them.

---

## 1. Missing Blackboard Key

**Symptom**: Condition evaluation fails silently.

**Cause**: Transition condition references a blackboard key that doesn't exist.

**Detection**:
```cpp
if (!GetBlackboardValue(Condition.Name, CurrentValue))
{
    UE_LOG(LogEAIS, Warning, TEXT("Missing blackboard key: %s"), *Condition.Name);
    return false;
}
```

**Resolution**:
1. Add key to blackboard defaults in JSON
2. Set key at runtime before checking

---

## 2. Invalid Transition

**Symptom**: State never transitions despite condition being met.

**Causes**:
- Target state doesn't exist
- Typo in state ID
- Circular transition without delay

**Detection**:
```cpp
const FAIState* NewState = GetState(StateId);
if (!NewState)
{
    UE_LOG(LogEAIS, Error, TEXT("Invalid transition target: %s"), *StateId);
}
```

**Resolution**:
1. Verify state ID spelling
2. Check that target state exists in JSON
3. For loops, add timer condition

---

## 3. No PlayerController

**Symptom**: Input injection fails (when using P_MEIS).

**Cause**: AI pawn doesn't have a PlayerController.

**Resolution**:
1. Ensure pawn has proper controller
2. Use game-specific input bypass

---

## 4. No TargetProvider

**Symptom**: MoveTo action has no destination.

**Cause**: Pawn doesn't implement IEAIS_TargetProvider interface.

**Resolution**:
1. Implement IEAIS_TargetProvider on your Pawn class
2. Or use absolute coordinates in params

---

## 5. Invalid JSON

**Symptom**: AI behavior doesn't load.

**Causes**:
- Syntax error in JSON
- Missing required fields
- Wrong data types

**Resolution**:
1. Run ValidateAIJson.ps1
2. Use JSON validator
3. Check for missing commas, quotes

---

## 6. Action Not Registered

**Symptom**: Action silently fails.

**Cause**: Action name in JSON doesn't match registered action.

**Resolution**:
1. Register action in EAISSubsystem
2. Check action name spelling
3. Update EAIS_ActionManifest.json

---

## 7. Infinite Loop

**Symptom**: Game freezes or AI oscillates rapidly.

**Cause**: State transitions back and forth without delay.

**Resolution**:
1. Add timer condition to one transition
2. Add blocking blackboard condition
3. Use terminal states

---

## 8. Hot Reload Issues

**Symptom**: Crash or undefined behavior after hot reload.

**Resolution**:
1. Restart PIE session
2. Avoid hot reload with active AI

---

## 9. Network Desync

**Symptom**: AI behaves differently on client vs server.

**Cause**: Client attempting to run AI logic.

**Resolution**:
1. Ensure AI only ticks on server:
```cpp
if (!GetOwner()->HasAuthority()) return;
```

---

## Error Handling Best Practices

1. **Log all failures** with context
2. **Fail fast** on critical errors
3. **Graceful degradation** for non-critical
4. **Use asserts** in development builds
5. **CI validation** catches most issues

---

## Debugging Commands

```
EAIS.Debug 1              # Enable debug overlay
EAIS.ListActions          # Show registered actions
EAIS.InjectEvent * Test   # Test event handling
```

---

*P_EAIS - Modular AI System*
