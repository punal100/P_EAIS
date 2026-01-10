/*
 * @Author: Punal Manalan
 * @Description: Implementation of FAIInterpreter
 * @Date: 29/12/2025
 */

#include "AIInterpreter.h"
#include "AIBehaviour.h"
#include "AIComponent.h"
#include "AIAction.h"
#include "EAISSubsystem.h"
#include "Engine/World.h"

FAIInterpreter::FAIInterpreter()
{
}

bool FAIInterpreter::LoadFromJson(const FString& JsonString, FString& OutError)
{
    // Create a temporary UAIBehaviour to parse
    UAIBehaviour* TempBehavior = NewObject<UAIBehaviour>();
    TempBehavior->EmbeddedJson = JsonString;
    
    if (!TempBehavior->ParseBehavior(OutError))
    {
        return false;
    }

    return LoadFromDef(TempBehavior->GetBehaviorDef());
}

bool FAIInterpreter::LoadFromDef(const FAIBehaviorDef& InBehaviorDef)
{
    if (!InBehaviorDef.bIsValid)
    {
        return false;
    }

    BehaviorDef = InBehaviorDef;

    // Initialize blackboard with default values from behavior definition
    for (const FEAISBlackboardEntry& Entry : BehaviorDef.Blackboard)
    {
        FBlackboardValue Value;
        Value.Type = Entry.Value.Type;
        Value.RawValue = Entry.Value.RawValue;
        
        // Parse from raw value based on type
        switch (Entry.Value.Type)
        {
        case EBlackboardValueType::Bool:
            Value.BoolValue = Entry.Value.RawValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
            break;
        case EBlackboardValueType::Int:
            Value.IntValue = FCString::Atoi(*Entry.Value.RawValue);
            break;
        case EBlackboardValueType::Float:
            Value.FloatValue = FCString::Atof(*Entry.Value.RawValue);
            break;
        case EBlackboardValueType::String:
            Value.StringValue = Entry.Value.RawValue;
            break;
        case EBlackboardValueType::Vector:
            Value.VectorValue.InitFromString(Entry.Value.RawValue);
            break;
        default:
            Value.StringValue = Entry.Value.RawValue;
            break;
        }

        Blackboard.Add(Entry.Key, Value);
    }

    return true;
}

void FAIInterpreter::Initialize(UAIComponent* OwnerComp)
{
    OwnerComponent = OwnerComp;
    Reset();
}

void FAIInterpreter::Reset()
{
    CurrentStateId.Empty();
    PreviousStateId.Empty();
    EventQueue.Empty();
    RecentEvents.Empty();
    TimerValues.Empty();
    StateElapsedTime = 0.0f;
    TotalRuntime = 0.0f;
    bIsPaused = false;

    // Reinitialize blackboard
    Blackboard.Empty();
    for (const FEAISBlackboardEntry& Entry : BehaviorDef.Blackboard)
    {
        FBlackboardValue Value;
        Value.Type = Entry.Value.Type;
        Value.RawValue = Entry.Value.RawValue;
        
        if (Entry.Value.Type == EBlackboardValueType::Bool)
        {
            Value.BoolValue = Entry.Value.RawValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
        }
        else if (Entry.Value.Type == EBlackboardValueType::Int)
        {
            Value.IntValue = FCString::Atoi(*Entry.Value.RawValue);
        }
        else if (Entry.Value.Type == EBlackboardValueType::Float)
        {
            Value.FloatValue = FCString::Atof(*Entry.Value.RawValue);
        }
        else
        {
            Value.StringValue = Entry.Value.RawValue;
        }
        Blackboard.Add(Entry.Key, Value);
    }

    // Enter initial state
    if (!BehaviorDef.InitialState.IsEmpty())
    {
        EnterState(BehaviorDef.InitialState);
    }
}

void FAIInterpreter::Tick(float DeltaSeconds)
{
    if (bIsPaused && !bShouldStep)
    {
        return;
    }

    bShouldStep = false;

    if (!IsValid())
    {
        return;
    }

    // Update timers
    StateElapsedTime += DeltaSeconds;
    TotalRuntime += DeltaSeconds;
    
    for (auto& Pair : TimerValues)
    {
        Pair.Value += DeltaSeconds;
    }

    // Process events
    ProcessEvents();

    // Get current state
    const FAIState* CurrentState = GetState(CurrentStateId);
    if (!CurrentState)
    {
        return;
    }

    // Execute OnTick actions
    ExecuteActions(CurrentState->OnTick);

    // Evaluate transitions (sorted by priority)
    TArray<FAITransition> SortedTransitions = CurrentState->Transitions;
    SortedTransitions.Sort([](const FAITransition& A, const FAITransition& B)
    {
        return A.Priority > B.Priority;
    });

    for (const FAITransition& Trans : SortedTransitions)
    {
        if (EvaluateCondition(Trans.Condition))
        {
            ForceTransition(Trans.To);
            break;
        }
    }

    // Clear recent events after tick
    ClearRecentEvents();
}

void FAIInterpreter::EnqueueEvent(const FString& EventName, const FAIEventPayload& Payload)
{
    FAIQueuedEvent Event;
    Event.EventName = EventName;
    Event.Payload = Payload;
    Event.QueuedTime = TotalRuntime;
    EventQueue.Add(Event);
}

bool FAIInterpreter::ForceTransition(const FString& StateId)
{
    if (StateId.IsEmpty() || StateId == CurrentStateId)
    {
        return false;
    }

    const FAIState* NewState = GetState(StateId);
    if (!NewState)
    {
        UE_LOG(LogTemp, Warning, TEXT("FAIInterpreter: Cannot transition to unknown state '%s'"), *StateId);
        return false;
    }

    ExitState();
    EnterState(StateId);
    return true;
}

void FAIInterpreter::SetPaused(bool bPause)
{
    bIsPaused = bPause;
}

void FAIInterpreter::StepTick()
{
    bShouldStep = true;
}

void FAIInterpreter::SetBlackboardValue(const FString& Key, const FBlackboardValue& Value)
{
    Blackboard.Add(Key, Value);
}

bool FAIInterpreter::GetBlackboardValue(const FString& Key, FBlackboardValue& OutValue) const
{
    const FBlackboardValue* Found = Blackboard.Find(Key);
    if (Found)
    {
        OutValue = *Found;
        return true;
    }
    return false;
}

void FAIInterpreter::SetBlackboardBool(const FString& Key, bool Value)
{
    SetBlackboardValue(Key, FBlackboardValue(Value));
}

bool FAIInterpreter::GetBlackboardBool(const FString& Key) const
{
    FBlackboardValue Value;
    if (GetBlackboardValue(Key, Value))
    {
        return Value.BoolValue;
    }
    return false;
}

void FAIInterpreter::SetBlackboardFloat(const FString& Key, float Value)
{
    SetBlackboardValue(Key, FBlackboardValue(Value));
}

float FAIInterpreter::GetBlackboardFloat(const FString& Key) const
{
    FBlackboardValue Value;
    if (GetBlackboardValue(Key, Value))
    {
        return Value.FloatValue;
    }
    return 0.0f;
}

void FAIInterpreter::SetBlackboardVector(const FString& Key, const FVector& Value)
{
    SetBlackboardValue(Key, FBlackboardValue(Value));
}

FVector FAIInterpreter::GetBlackboardVector(const FString& Key) const
{
    FBlackboardValue Value;
    if (GetBlackboardValue(Key, Value))
    {
        return Value.VectorValue;
    }
    return FVector::ZeroVector;
}

void FAIInterpreter::SetBlackboardObject(const FString& Key, UObject* Value)
{
    SetBlackboardValue(Key, FBlackboardValue(Value));
}

UObject* FAIInterpreter::GetBlackboardObject(const FString& Key) const
{
    FBlackboardValue Value;
    if (GetBlackboardValue(Key, Value) && Value.ObjectValue.IsValid())
    {
        return Value.ObjectValue.Get();
    }
    return nullptr;
}

TArray<FString> FAIInterpreter::GetAllStateIds() const
{
    TArray<FString> Result;
    for (const FAIState& State : BehaviorDef.States)
    {
        Result.Add(State.Id);
    }
    return Result;
}

const FAIState* FAIInterpreter::GetState(const FString& StateId) const
{
    for (const FAIState& State : BehaviorDef.States)
    {
        if (State.Id == StateId)
        {
            return &State;
        }
    }
    return nullptr;
}

void FAIInterpreter::EnterState(const FString& StateId)
{
    FString OldState = CurrentStateId;
    CurrentStateId = StateId;
    StateElapsedTime = 0.0f;

    // Reset timer for this state
    TimerValues.Add(StateId, 0.0f);

    UE_LOG(LogTemp, Verbose, TEXT("FAIInterpreter: Entering state '%s'"), *StateId);

    const FAIState* State = GetState(StateId);
    if (State)
    {
        ExecuteActions(State->OnEnter);
    }

    // Broadcast state change
    if (OnStateChanged.IsBound())
    {
        OnStateChanged.Broadcast(OldState, StateId);
    }
}

void FAIInterpreter::ExitState()
{
    if (CurrentStateId.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Verbose, TEXT("FAIInterpreter: Exiting state '%s'"), *CurrentStateId);

    const FAIState* State = GetState(CurrentStateId);
    if (State)
    {
        ExecuteActions(State->OnExit);
    }

    PreviousStateId = CurrentStateId;
}

void FAIInterpreter::ExecuteActions(const TArray<FAIActionEntry>& Actions)
{
    if (!OwnerComponent.IsValid())
    {
        return;
    }

    UWorld* World = OwnerComponent->GetWorld();
    if (!World)
    {
        return;
    }

    UEAISSubsystem* Subsystem = World->GetGameInstance()->GetSubsystem<UEAISSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    for (const FAIActionEntry& Entry : Actions)
    {
        UAIAction* Action = Subsystem->GetAction(Entry.Action);
        if (Action)
        {
            Action->Execute(OwnerComponent.Get(), Entry.Params);

            if (OnActionExecuted.IsBound())
            {
                OnActionExecuted.Broadcast(Entry.Action, Entry.Params);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FAIInterpreter: Unknown action '%s'"), *Entry.Action);
        }
    }
}

bool FAIInterpreter::EvaluateCondition(const FAICondition& Condition) const
{
    switch (Condition.Type)
    {
    case EAIConditionType::Blackboard:
        {
            FBlackboardValue CurrentValue;
            if (!GetBlackboardValue(Condition.Name, CurrentValue))
            {
                // Key doesn't exist - check if the condition name itself is a key
                // This handles simple conditions like "HasBall" which means "HasBall == true"
                if (!GetBlackboardValue(Condition.Name, CurrentValue))
                {
                    return false;
                }
            }

            // Parse the comparison value
            FBlackboardValue CompareValue = CurrentValue;  // Same type
            CompareValue.FromString(Condition.Value);

            return CurrentValue.Compare(CompareValue, Condition.Operator);
        }

    case EAIConditionType::Event:
        return RecentEvents.Contains(Condition.Name);

    case EAIConditionType::Timer:
        {
            const float* Timer = TimerValues.Find(CurrentStateId);
            if (Timer)
            {
                return *Timer >= Condition.Seconds;
            }
            return StateElapsedTime >= Condition.Seconds;
        }

    case EAIConditionType::Distance:
    {
        FVector TargetLocation = FVector::ZeroVector;
        
        // Try to get location from blackboard
        FBlackboardValue BBVal;
        FString BBKey = Condition.Target.IsEmpty() ? Condition.Name : Condition.Target;

        if (GetBlackboardValue(BBKey, BBVal))
        {
            if (BBVal.Type == EBlackboardValueType::Vector)
            {
                TargetLocation = BBVal.VectorValue;
            }
            else if (BBVal.Type == EBlackboardValueType::Object && BBVal.ObjectValue.IsValid())
            {
                AActor* TargetActor = Cast<AActor>(BBVal.ObjectValue.Get());
                if (TargetActor)
                {
                    TargetLocation = TargetActor->GetActorLocation();
                }
            }
        }
        else if (BBKey.Equals(TEXT("Ball"), ESearchCase::IgnoreCase))
        {
            // Specialized/Temporary fallback for ball if not in BB
        }

        AActor* Owner = OwnerComponent->GetOwner();
        if (Owner)
        {
            float Distance = FVector::Dist(Owner->GetActorLocation(), TargetLocation);
            float CompareDistance = FCString::Atof(*Condition.Value);

            switch (Condition.Operator)
            {
            case EAIConditionOperator::Equal: return FMath::IsNearlyEqual(Distance, CompareDistance, 10.0f);
            case EAIConditionOperator::NotEqual: return !FMath::IsNearlyEqual(Distance, CompareDistance, 10.0f);
            case EAIConditionOperator::GreaterThan: return Distance > CompareDistance;
            case EAIConditionOperator::LessThan: return Distance < CompareDistance;
            case EAIConditionOperator::GreaterOrEqual: return Distance >= CompareDistance;
            case EAIConditionOperator::LessOrEqual: return Distance <= CompareDistance;
            }
        }
        return false;
    }

    case EAIConditionType::And:
    {
        for (const FAICondition& Sub : Condition.SubConditions)
        {
            if (!EvaluateCondition(Sub)) return false;
        }
        return Condition.SubConditions.Num() > 0;
    }

    case EAIConditionType::Or:
    {
        for (const FAICondition& Sub : Condition.SubConditions)
        {
            if (EvaluateCondition(Sub)) return true;
        }
        return false;
    }

    case EAIConditionType::Not:
    {
        if (Condition.SubConditions.Num() > 0)
        {
            return !EvaluateCondition(Condition.SubConditions[0]);
        }
        return false;
    }

    case EAIConditionType::Custom:
        // Registered C++ conditions would go here
        return false;

    default:
        return false;
    }
}

void FAIInterpreter::ProcessEvents()
{
    // Move events to recent events set for condition checking
    for (const FAIQueuedEvent& Event : EventQueue)
    {
        RecentEvents.Add(Event.EventName);
        
        // Also set blackboard values from event payload
        for (const auto& Pair : Event.Payload.StringParams)
        {
            SetBlackboardValue(Pair.Key, FBlackboardValue(Pair.Value));
        }
        for (const auto& Pair : Event.Payload.FloatParams)
        {
            SetBlackboardValue(Pair.Key, FBlackboardValue(Pair.Value));
        }
        for (const auto& Pair : Event.Payload.VectorParams)
        {
            SetBlackboardValue(Pair.Key, FBlackboardValue(Pair.Value));
        }
    }

    EventQueue.Empty();
}

void FAIInterpreter::ClearRecentEvents()
{
    RecentEvents.Empty();
}
