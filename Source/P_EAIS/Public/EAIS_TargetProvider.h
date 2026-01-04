// Copyright Punal Manalan. All Rights Reserved.
// Game-agnostic target resolution interface

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EAIS_TargetProvider.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UEAIS_TargetProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * Implement this interface on your Pawn/Character to provide targets to AI.
 * This decouples P_EAIS from game-specific classes.
 * 
 * Example implementation in your game's Character:
 * @code
 * class AMyCharacter : public ACharacter, public IEAIS_TargetProvider
 * {
 *     virtual FVector ResolveTargetLocation_Implementation(const FString& TargetName) const override
 *     {
 *         if (TargetName == "ball") return GetBallLocation();
 *         if (TargetName == "opponentGoal") return GetOpponentGoalLocation();
 *         return FVector::ZeroVector;
 *     }
 * };
 * @endcode
 */
class P_EAIS_API IEAIS_TargetProvider
{
    GENERATED_BODY()

public:
    /**
     * Resolve a target name (e.g., "ball", "opponentGoal") to a world location.
     * @param TargetName Logical name of the target
     * @return World location of the target, or ZeroVector if not found
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    FVector ResolveTargetLocation(const FString& TargetName) const;
    virtual FVector ResolveTargetLocation_Implementation(const FString& TargetName) const { return FVector::ZeroVector; }
    
    /**
     * Resolve a target name to an actor.
     * @param TargetName Logical name of the target
     * @return Actor for the target, or nullptr if not found
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    AActor* ResolveTargetActor(const FString& TargetName) const;
    virtual AActor* ResolveTargetActor_Implementation(const FString& TargetName) const { return nullptr; }
    
    /**
     * Get the current team ID for this AI.
     * @return Team ID (0 = no team)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    int32 GetTeamId() const;
    virtual int32 GetTeamId_Implementation() const { return 0; }
    
    /**
     * Get the current role for this AI.
     * @return Role name (e.g., "Striker", "Defender")
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    FString GetRole() const;
    virtual FString GetRole_Implementation() const { return TEXT(""); }
};
