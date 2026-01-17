/*
 * @Author: Punal Manalan
 * @Description: Implementation of AI Actions
 * @Date: 29/12/2025
 */

#include "AIAction.h"
#include "AIComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EAIS_TargetProvider.h"
#include "EAIS_ActionExecutor.h"
#include "UObject/UObjectIterator.h"

// Include P_MEIS for input injection
#include "Manager/CPP_BPL_InputBinding.h"

void UAIAction::Complete()
{
    bIsRunning = false;
}

// ==================== MoveTo ====================

void UAIAction_MoveTo::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    UWorld* World = OwnerComponent->GetWorld();
    if (!World)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    AAIController* AIController = Cast<AAIController>(Pawn->GetController());
    if (!AIController)
    {
        return;
    }

    SetRunning(true);

    // Get target location
    FVector TargetLocation = FVector::ZeroVector;
    bool bTargetFound = false;

    // 1. Try TargetProvider
    if (IEAIS_TargetProvider* TargetProvider = Cast<IEAIS_TargetProvider>(Pawn))
    {
        bTargetFound = IEAIS_TargetProvider::Execute_EAIS_GetTargetLocation(Pawn, FName(*Params.Target), TargetLocation);
    }

    // 2. Fallback to basic tag-based or blackboard
    if (!bTargetFound)
    {
        // Check if target is "ball" - special case
        if (Params.Target.Equals(TEXT("ball"), ESearchCase::IgnoreCase))
        {
            // Find ball actor
            TArray<AActor*> FoundActors;
            UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Ball")), FoundActors);
            if (FoundActors.Num() > 0)
            {
                TargetLocation = FoundActors[0]->GetActorLocation();
                bTargetFound = true;
            }
        }
        else if (Params.Target.StartsWith(TEXT("(")))
        {
            // Parse vector string
            TargetLocation.InitFromString(Params.Target);
            bTargetFound = true;
        }
        else
        {
            // Try to get from blackboard
            TargetLocation = OwnerComponent->GetBlackboardVector(Params.Target);
            bTargetFound = !TargetLocation.IsZero();
        }
    }

    if (!bTargetFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("UAIAction_MoveTo: Could not resolve target '%s'"), *Params.Target);
        return;
    }

    // Move to target
    float AcceptanceRadius = 50.0f;

    // Avoid repeatedly spamming MoveTo every tick for essentially the same destination.
    // This is especially important for crowd/avoidance and to prevent visible stutter.
    {
        static const FString LastTargetKey = TEXT("__EAIS_LastMoveToTarget");
        static const FString LastTimeKey = TEXT("__EAIS_LastMoveToTime");
        constexpr float MinRetargetDistance = 25.0f;  // cm
        constexpr float MinRetargetInterval = 0.15f;  // seconds

        const FVector LastTarget = OwnerComponent->GetBlackboardVector(LastTargetKey);
        const float LastTime = OwnerComponent->GetBlackboardFloat(LastTimeKey);
        const float Now = World->GetTimeSeconds();

        const bool bHasLast = !LastTarget.IsZero() && LastTime > 0.0f;
        const bool bSameTarget = bHasLast && FVector::DistSquared(LastTarget, TargetLocation) <= FMath::Square(MinRetargetDistance);
        const bool bTooSoon = bHasLast && (Now - LastTime) < MinRetargetInterval;

        if (bSameTarget && bTooSoon)
        {
            return;
        }

        OwnerComponent->SetBlackboardVector(LastTargetKey, TargetLocation);
        OwnerComponent->SetBlackboardFloat(LastTimeKey, Now);
    }

    EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(TargetLocation, AcceptanceRadius, true, true, true, true);
    
    if (Result == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Error, TEXT("UAIAction_MoveTo: MoveToLocation FAILED for %s -> %s. NavMesh might be missing or target unreachable."), *Pawn->GetName(), *TargetLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("UAIAction_MoveTo: MoveToLocation Request: %s"), *UEnum::GetValueAsString(Result));
    }

    PathFollowingComp = AIController->GetPathFollowingComponent();
}

void UAIAction_MoveTo::Abort_Implementation()
{
    if (PathFollowingComp.IsValid())
    {
        PathFollowingComp->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
    }
    SetRunning(false);
}

// ==================== Log ====================

void UAIAction_Log::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    FString Message = Params.ExtraParams.FindRef(TEXT("message"));
    if (Message.IsEmpty())
    {
        Message = Params.Target;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("AI_LOG [%s]: %s"), *OwnerComponent->GetOwner()->GetName(), *Message);
    Complete();
}

void UAIAction_Log::Abort_Implementation()
{
    SetRunning(false);
}

// ==================== Kick ====================

void UAIAction_Kick::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent || !OwnerComponent->GetOwnerPawn()) return;
    APawn* Pawn = OwnerComponent->GetOwnerPawn();

    FVector Direction = Pawn->GetActorForwardVector();
    float Power = Params.Power;

    // Use Reflection to call ExecuteShoot(FVector Direction, float Power)
    UFunction* Function = Pawn->FindFunction(FName(TEXT("ExecuteShoot")));
    if (Function)
    {
        struct FExecuteShootParams
        {
            FVector Direction;
            float Power;
        };
        FExecuteShootParams FuncParams;
        FuncParams.Direction = Direction;
        FuncParams.Power = Power;

        Pawn->ProcessEvent(Function, &FuncParams);
        UE_LOG(LogTemp, Verbose, TEXT("UAIAction_Kick: Invoked ExecuteShoot via reflection"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAIAction_Kick: Could not find 'ExecuteShoot' function on pawn %s"), *Pawn->GetName());
    }

    OwnerComponent->SetBlackboardFloat(TEXT("KickPower"), Params.Power);
}

// ==================== AimAt ====================

void UAIAction_AimAt::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    FVector TargetLocation = FVector::ZeroVector;

    // Check for special targets
    if (Params.Target.Equals(TEXT("opponentGoal"), ESearchCase::IgnoreCase))
    {
        // Find opponent goal
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Goal")), FoundActors);
        
        // TODO: Determine which goal is opponent's based on team
        if (FoundActors.Num() > 0)
        {
            TargetLocation = FoundActors[0]->GetActorLocation();
        }
    }
    else
    {
        TargetLocation = OwnerComponent->GetBlackboardVector(Params.Target);
    }

    // Set focus/aim direction
    AAIController* AIController = Cast<AAIController>(Pawn->GetController());
    if (AIController)
    {
        AIController->SetFocalPoint(TargetLocation);
    }

    // Store aim target in blackboard
    OwnerComponent->SetBlackboardVector(TEXT("AimTarget"), TargetLocation);
}

// ==================== SetLookTarget ====================

void UAIAction_SetLookTarget::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    AAIController* AIController = Cast<AAIController>(Pawn->GetController());
    if (!AIController)
    {
        return;
    }

    if (Params.Target.Equals(TEXT("nearest_enemy"), ESearchCase::IgnoreCase))
    {
        // Find nearest enemy
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Player")), FoundActors);
        
        AActor* Nearest = nullptr;
        float NearestDist = FLT_MAX;
        FVector MyLocation = Pawn->GetActorLocation();

        for (AActor* Actor : FoundActors)
        {
            if (Actor != Pawn)
            {
                float Dist = FVector::Dist(MyLocation, Actor->GetActorLocation());
                if (Dist < NearestDist)
                {
                    NearestDist = Dist;
                    Nearest = Actor;
                }
            }
        }

        if (Nearest)
        {
            AIController->SetFocus(Nearest);
        }
    }
    else if (Params.Target.Equals(TEXT("ball"), ESearchCase::IgnoreCase))
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Ball")), FoundActors);
        if (FoundActors.Num() > 0)
        {
            AIController->SetFocus(FoundActors[0]);
        }
    }
}

// ==================== Wait ====================

void UAIAction_Wait::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    // Wait is passive - the interpreter handles timer-based transitions
    // This action just sets a flag
    if (OwnerComponent)
    {
        OwnerComponent->SetBlackboardFloat(TEXT("WaitTime"), Params.Power);
    }
}

// ==================== SetBlackboardKey ====================

void UAIAction_SetBlackboardKey::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    // Target is the key name, look for value in ExtraParams
    FString Key = Params.Target;
    
    if (Params.ExtraParams.Contains(TEXT("value")))
    {
        FString ValueStr = Params.ExtraParams[TEXT("value")];
        
        // Try to detect type
        if (ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
            ValueStr.Equals(TEXT("false"), ESearchCase::IgnoreCase))
        {
            OwnerComponent->SetBlackboardBool(Key, ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase));
        }
        else if (ValueStr.IsNumeric())
        {
            OwnerComponent->SetBlackboardFloat(Key, FCString::Atof(*ValueStr));
        }
        else
        {
            OwnerComponent->SetBlackboardValue(Key, FBlackboardValue(ValueStr));
        }
    }
}

// ==================== InjectInput ====================

void UAIAction_InjectInput::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
    if (!PC)
    {
        return;
    }

    // Target is the action name to inject
    FName ActionName = FName(*Params.Target);
    
    // Check for trigger type in extra params
    FString TriggerType = TEXT("Triggered");
    if (Params.ExtraParams.Contains(TEXT("trigger")))
    {
        TriggerType = Params.ExtraParams[TEXT("trigger")];
    }

    if (TriggerType.Equals(TEXT("Started"), ESearchCase::IgnoreCase))
    {
        UCPP_BPL_InputBinding::InjectActionStarted(PC, ActionName);
    }
    else if (TriggerType.Equals(TEXT("Completed"), ESearchCase::IgnoreCase))
    {
        UCPP_BPL_InputBinding::InjectActionCompleted(PC, ActionName);
    }
    else
    {
        UCPP_BPL_InputBinding::InjectActionTriggered(PC, ActionName);
    }
}

// ==================== PassToTeammate ====================

void UAIAction_PassToTeammate::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    // Find nearest teammate
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Player")), FoundActors);

    AActor* NearestTeammate = nullptr;
    float NearestDist = FLT_MAX;
    FVector MyLocation = Pawn->GetActorLocation();

    // Get my team from blackboard
    FString MyTeam = OwnerComponent->GetBlackboardValue(TEXT("Team")).StringValue;

    for (AActor* Actor : FoundActors)
    {
        if (Actor != Pawn)
        {
            // TODO: Check if same team
            float Dist = FVector::Dist(MyLocation, Actor->GetActorLocation());
            if (Dist < NearestDist)
            {
                NearestDist = Dist;
                NearestTeammate = Actor;
            }
        }
    }

    if (NearestTeammate)
    {
        // Aim at teammate and kick
        AAIController* AIController = Cast<AAIController>(Pawn->GetController());
        if (AIController)
        {
            AIController->SetFocus(NearestTeammate);
        }

        // Inject kick with lower power
        APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
        if (PC)
        {
            OwnerComponent->SetBlackboardFloat(TEXT("KickPower"), 0.5f);
            UCPP_BPL_InputBinding::InjectActionTriggered(PC, FName(TEXT("Kick")));
        }
    }
}

// ==================== LookAround ====================

void UAIAction_LookAround::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent)
    {
        return;
    }

    APawn* Pawn = OwnerComponent->GetOwnerPawn();
    if (!Pawn)
    {
        return;
    }

    AAIController* AIController = Cast<AAIController>(Pawn->GetController());
    if (AIController)
    {
        // Clear focus to look around freely
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
    }
}

// ==================== Execute ====================

void UAIAction_Execute::Execute_Implementation(UAIComponent* OwnerComponent, const FAIActionParams& Params)
{
    if (!OwnerComponent) return;
    AActor* Owner = OwnerComponent->GetOwner();
    if (!Owner) return;

    // Look for ActionExecutor in owner actor components
    IEAIS_ActionExecutor* Executor = Cast<IEAIS_ActionExecutor>(Owner);
    if (!Executor)
    {
        // Try components
        TArray<UActorComponent*> Components;
        Owner->GetComponents(Components);
        for (UActorComponent* Comp : Components)
        {
            if (Comp->GetClass()->ImplementsInterface(UEAIS_ActionExecutor::StaticClass()))
            {
                Executor = Cast<IEAIS_ActionExecutor>(Comp);
                break;
            }
        }
    }

    if (Executor)
    {
        // Construct Inner Params for the action being executed
        FAIActionParams InnerParams;
        
        // Populate InnerParams from the ExtraParams of the Execute action
        // This assumes the Parser has flattened necessary nested params into ExtraParams
        if (Params.ExtraParams.Contains(TEXT("target")))
        {
            InnerParams.Target = Params.ExtraParams[TEXT("target")];
        }
        
        if (Params.ExtraParams.Contains(TEXT("power")))
        {
            InnerParams.Power = FCString::Atof(*Params.ExtraParams[TEXT("power")]);
        }
        else
        {
            // Default power passed down? Or use default 1.0
            InnerParams.Power = 1.0f;
        }

        // Copy all ExtraParams to InnerParams.ExtraParams so the callee has access to everything
        InnerParams.ExtraParams = Params.ExtraParams;

        // Remove the mapped keys to avoid confusion? No, keep them for flexibility.

        // Pass the actual object that implements the interface (either the Owner or the Component)
        UObject* ImplementingObject = Cast<UObject>(Executor);
        IEAIS_ActionExecutor::Execute_EAIS_ExecuteAction(ImplementingObject, FName(*Params.Target), InnerParams);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAIAction_Execute: No IEAIS_ActionExecutor found on %s"), *Owner->GetName());
    }

    Complete();
}
