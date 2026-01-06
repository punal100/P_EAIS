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
 */
class P_EAIS_API IEAIS_TargetProvider
{
    GENERATED_BODY()

public:
    /**
     * Resolve a target ID (e.g., "Ball", "Goal_Opponent") to a world location.
     * @param TargetId Logical ID of the target
     * @param OutLocation Resulting world location
     * @return True if target was resolved successfully
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    bool EAIS_GetTargetLocation(FName TargetId, FVector& OutLocation) const;
    virtual bool EAIS_GetTargetLocation_Implementation(FName TargetId, FVector& OutLocation) const { return false; }
    
    /**
     * Resolve a target ID to an actor.
     * @param TargetId Logical ID of the target
     * @param OutActor Resulting actor
     * @return True if target was resolved successfully
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    bool EAIS_GetTargetActor(FName TargetId, AActor*& OutActor) const;
    virtual bool EAIS_GetTargetActor_Implementation(FName TargetId, AActor*& OutActor) const { return false; }

    
    /**
     * Get the current team ID for this AI.
     * @return Team ID (0 = no team)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    int32 EAIS_GetTeamId() const;
    virtual int32 EAIS_GetTeamId_Implementation() const { return 0; }
    
    /**
     * Get the current role for this AI.
     * @return Role name (e.g., "Striker", "Defender")
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    FString EAIS_GetRole() const;
    virtual FString EAIS_GetRole_Implementation() const { return TEXT(""); }
};

