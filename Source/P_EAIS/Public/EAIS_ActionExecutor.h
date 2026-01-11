// Copyright Punal Manalan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EAIS_Types.h"
#include "EAIS_ActionExecutor.generated.h"

/** Result of an action execution */
USTRUCT(BlueprintType)
struct FEAIS_ActionResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EAIS")
    FString Message;
};

UINTERFACE(BlueprintType, MinimalAPI)
class UEAIS_ActionExecutor : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for game-specific components that execute AI actions.
 * Typically implemented by a Component attached to the Pawn/AI.
 */
class P_EAIS_API IEAIS_ActionExecutor
{
    GENERATED_BODY()

public:
    /**
     * Execute an action by its logical ID.
     * @param ActionId Logical ID (e.g., "MF.Shoot")
     * @param Params Parameters struct
     * @return Execution result
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EAIS")
    FEAIS_ActionResult EAIS_ExecuteAction(const FName ActionId, const FAIActionParams& Params);
    virtual FEAIS_ActionResult EAIS_ExecuteAction_Implementation(const FName ActionId, const FAIActionParams& Params) 
    { 
        return FEAIS_ActionResult(); 
    }
};
