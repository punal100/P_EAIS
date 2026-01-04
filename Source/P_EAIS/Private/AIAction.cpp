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
    
    // Check if target is "ball" - special case
    if (Params.Target.Equals(TEXT("ball"), ESearchCase::IgnoreCase))
    {
        // Find ball actor
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Ball")), FoundActors);
        if (FoundActors.Num() > 0)
        {
            TargetLocation = FoundActors[0]->GetActorLocation();
        }
    }
    // Check for "opponentGoal"
    else if (Params.Target.Equals(TEXT("opponentGoal"), ESearchCase::IgnoreCase))
    {
        // Find goals
        TArray<AActor*> FoundGoals;
        UGameplayStatics::GetAllActorsWithTag(OwnerComponent->GetWorld(), FName(TEXT("Goal")), FoundGoals);
        
        // Determine opponent goal based on team
        float TeamIDVal = OwnerComponent->GetBlackboardFloat(TEXT("TeamID"));
        int32 MyTeam = FMath::RoundToInt(TeamIDVal); // 1=TeamA, 2=TeamB
        
        // Logic: Team A (1) Defends +Y, Attacks -Y
        //        Team B (2) Defends -Y, Attacks +Y
        
        for (AActor* Goal : FoundGoals)
        {
            if (MyTeam == 1 && Goal->GetActorLocation().Y < 0) { TargetLocation = Goal->GetActorLocation(); break; }
            if (MyTeam == 2 && Goal->GetActorLocation().Y > 0) { TargetLocation = Goal->GetActorLocation(); break; }
        }
    }
    else if (Params.Target.StartsWith(TEXT("(")))
    {
        // Parse vector string
        TargetLocation.InitFromString(Params.Target);
    }
    else
    {
        // Try to get from blackboard
        TargetLocation = OwnerComponent->GetBlackboardVector(Params.Target);
    }

    // Move to target
    float AcceptanceRadius = 50.0f;
    UE_LOG(LogTemp, Warning, TEXT("UAIAction_MoveTo: Moving to %s (Cmd: %s)"), *TargetLocation.ToString(), *Params.Target);
    
    if (TargetLocation.IsZero())
    {
         UE_LOG(LogTemp, Warning, TEXT("UAIAction_MoveTo: TargetLocation is ZERO! Target param was: '%s'"), *Params.Target);
    }

    EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(TargetLocation, AcceptanceRadius, true, true, false, true);
    UE_LOG(LogTemp, Warning, TEXT("UAIAction_MoveTo: MoveToLocation Result: %d (0=Failed, 1=AlreadyAtGoal, 2=RequestSuccessful)"), (int32)Result);

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
