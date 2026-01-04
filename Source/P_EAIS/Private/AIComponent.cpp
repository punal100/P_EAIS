/*
 * @Author: Punal Manalan
 * @Description: Implementation of UAIComponent
 * @Date: 29/12/2025
 */

#include "AIComponent.h"
#include "AIBehaviour.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

UAIComponent::UAIComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAIComponent::BeginPlay()
{
    Super::BeginPlay();

    // Bind to interpreter events
    Interpreter.OnStateChanged.AddDynamic(this, &UAIComponent::HandleStateChanged);

    // Initialize from asset or JSON file
    if (AIBehaviour)
    {
        InitializeAI(AIBehaviour);
    }
    else if (!JsonFilePath.IsEmpty())
    {
        FString FullPath = FPaths::ProjectContentDir() / TEXT("AIProfiles") / JsonFilePath;
        FString JsonContent;
        if (FFileHelper::LoadFileToString(JsonContent, *FullPath))
        {
            FString Error;
            InitializeAIFromJson(JsonContent, Error);
            if (!Error.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("UAIComponent: Failed to initialize from JSON: %s"), *Error);
            }
        }
    }

    if (bAutoStart && Interpreter.IsValid())
    {
        StartAI();
    }
}

void UAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsRunning || !ShouldRun())
    {
        return;
    }

    // Handle tick interval
    if (TickInterval > 0.0f)
    {
        TimeSinceLastTick += DeltaTime;
        if (TimeSinceLastTick < TickInterval)
        {
            return;
        }

        DeltaTime = TimeSinceLastTick;
        TimeSinceLastTick = 0.0f;
    }

    Interpreter.Tick(DeltaTime);
}

bool UAIComponent::InitializeAI(UAIBehaviour* Behavior)
{
    if (!Behavior)
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("UAIComponent: InitializeAI called for %s"), *Behavior->GetName());
    AIBehaviour = Behavior;

    FString Error;
    if (!Behavior->ParseBehavior(Error))
    {
        UE_LOG(LogTemp, Error, TEXT("UAIComponent: Failed to parse behavior: %s"), *Error);
        return false;
    }

    if (!Interpreter.LoadFromDef(Behavior->GetBehaviorDef()))
    {
        UE_LOG(LogTemp, Error, TEXT("UAIComponent: Failed to load interpreter definition"));
        return false;
    }

    Interpreter.Initialize(this);
    UE_LOG(LogTemp, Warning, TEXT("UAIComponent: AI initialized successfully."));
    return true;
}

bool UAIComponent::InitializeAIFromJson(const FString& JsonString, FString& OutError)
{
    UE_LOG(LogTemp, Warning, TEXT("UAIComponent: InitializeAIFromJson called. Length: %d"), JsonString.Len());
    if (!Interpreter.LoadFromJson(JsonString, OutError))
    {
        UE_LOG(LogTemp, Error, TEXT("UAIComponent: Failed to load from JSON: %s"), *OutError);
        return false;
    }

    Interpreter.Initialize(this);
    UE_LOG(LogTemp, Warning, TEXT("UAIComponent: AI initialized from JSON successfully."));
    return true;
}

void UAIComponent::StartAI()
{
    if (!Interpreter.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UAIComponent: StartAI called but interpreter invalid. Resetting."));
        Interpreter.Reset();
    }

    bIsRunning = true;
    Interpreter.SetPaused(false);

    UE_LOG(LogTemp, Warning, TEXT("UAIComponent: AI Started - %s (State: %s)"), *GetBehaviorName(), *GetCurrentState());
}

void UAIComponent::StopAI()
{
    bIsRunning = false;
    Interpreter.SetPaused(true);

    if (bDebugMode)
    {
        UE_LOG(LogTemp, Log, TEXT("UAIComponent: AI Stopped - %s"), *GetBehaviorName());
    }
}

void UAIComponent::ResetAI()
{
    Interpreter.Reset();
    TimeSinceLastTick = 0.0f;

    if (bDebugMode)
    {
        UE_LOG(LogTemp, Log, TEXT("UAIComponent: AI Reset - %s"), *GetBehaviorName());
    }
}

bool UAIComponent::ForceTransition(const FString& StateId)
{
    return Interpreter.ForceTransition(StateId);
}

void UAIComponent::EnqueueEvent(const FString& EventName, const FAIEventPayload& Payload)
{
    Interpreter.EnqueueEvent(EventName, Payload);
}

void UAIComponent::EnqueueSimpleEvent(const FString& EventName)
{
    FAIEventPayload Payload;
    Interpreter.EnqueueEvent(EventName, Payload);
}

void UAIComponent::SetBlackboardValue(const FString& Key, const FBlackboardValue& Value)
{
    Interpreter.SetBlackboardValue(Key, Value);
}

FBlackboardValue UAIComponent::GetBlackboardValue(const FString& Key) const
{
    FBlackboardValue Value;
    Interpreter.GetBlackboardValue(Key, Value);
    return Value;
}

void UAIComponent::SetBlackboardBool(const FString& Key, bool Value)
{
    Interpreter.SetBlackboardBool(Key, Value);
}

bool UAIComponent::GetBlackboardBool(const FString& Key) const
{
    return Interpreter.GetBlackboardBool(Key);
}

void UAIComponent::SetBlackboardFloat(const FString& Key, float Value)
{
    Interpreter.SetBlackboardFloat(Key, Value);
}

float UAIComponent::GetBlackboardFloat(const FString& Key) const
{
    return Interpreter.GetBlackboardFloat(Key);
}

void UAIComponent::SetBlackboardVector(const FString& Key, const FVector& Value)
{
    Interpreter.SetBlackboardVector(Key, Value);
}

FVector UAIComponent::GetBlackboardVector(const FString& Key) const
{
    return Interpreter.GetBlackboardVector(Key);
}

void UAIComponent::SetBlackboardObject(const FString& Key, UObject* Value)
{
    Interpreter.SetBlackboardObject(Key, Value);
}

UObject* UAIComponent::GetBlackboardObject(const FString& Key) const
{
    return Interpreter.GetBlackboardObject(Key);
}

FString UAIComponent::GetCurrentState() const
{
    return Interpreter.GetCurrentStateId();
}

FString UAIComponent::GetBehaviorName() const
{
    return Interpreter.GetBehaviorName();
}

bool UAIComponent::IsRunning() const
{
    return bIsRunning && !Interpreter.IsPaused();
}

bool UAIComponent::IsValid() const
{
    return Interpreter.IsValid();
}

float UAIComponent::GetStateElapsedTime() const
{
    return Interpreter.GetStateElapsedTime();
}

TArray<FString> UAIComponent::GetAllStates() const
{
    return Interpreter.GetAllStateIds();
}

APawn* UAIComponent::GetOwnerPawn() const
{
    AActor* Owner = GetOwner();
    
    // If owner is a pawn, return it
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        return Pawn;
    }

    // If owner is a controller, get its pawn
    if (AController* Controller = Cast<AController>(Owner))
    {
        return Controller->GetPawn();
    }

    return nullptr;
}

AController* UAIComponent::GetOwnerController() const
{
    AActor* Owner = GetOwner();
    
    // If owner is a controller, return it
    if (AController* Controller = Cast<AController>(Owner))
    {
        return Controller;
    }

    // If owner is a pawn, get its controller
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        return Pawn->GetController();
    }

    return nullptr;
}

void UAIComponent::HandleStateChanged(const FString& OldState, const FString& NewState)
{
    if (bDebugMode)
    {
        UE_LOG(LogTemp, Log, TEXT("UAIComponent [%s]: State Change %s -> %s"), 
            *GetBehaviorName(), *OldState, *NewState);
    }

    OnStateChanged.Broadcast(OldState, NewState);
}

bool UAIComponent::ShouldRun() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    switch (RunMode)
    {
    case EAIRunMode::Server:
        return Owner->HasAuthority();
    
    case EAIRunMode::Client:
        return !Owner->HasAuthority();
    
    case EAIRunMode::Both:
    default:
        return true;
    }
}
