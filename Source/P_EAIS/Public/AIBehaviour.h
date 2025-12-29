/*
 * @Author: Punal Manalan
 * @Description: UAIBehaviour - Primary asset type for AI behavior definitions
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EAIS_Types.h"
#include "AIBehaviour.generated.h"

/**
 * Primary asset type for AI behaviors.
 * Can either embed JSON directly or reference an external JSON file.
 */
UCLASS(BlueprintType)
class P_EAIS_API UAIBehaviour : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UAIBehaviour();

    // ==================== Asset Properties ====================

    /** Display name for this behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    FString BehaviorName;

    /** Embedded JSON string (used if JsonFilePath is empty) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior", meta = (MultiLine = true))
    FString EmbeddedJson;

    /** Path to external JSON file (relative to Content/AIProfiles) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Behavior")
    FString JsonFilePath;

    /** Parsed behavior definition (cached) */
    UPROPERTY(BlueprintReadOnly, Category = "AI Behavior")
    FAIBehaviorDef ParsedBehavior;

    // ==================== Runtime Methods ====================

    /** Get the JSON content (from embedded or file) */
    UFUNCTION(BlueprintCallable, Category = "AI Behavior")
    FString GetJsonContent() const;

    /** Parse the JSON and cache the behavior definition */
    UFUNCTION(BlueprintCallable, Category = "AI Behavior")
    bool ParseBehavior(FString& OutError);

    /** Get the cached parsed behavior */
    UFUNCTION(BlueprintPure, Category = "AI Behavior")
    const FAIBehaviorDef& GetBehaviorDef() const { return ParsedBehavior; }

    /** Check if behavior is valid */
    UFUNCTION(BlueprintPure, Category = "AI Behavior")
    bool IsValid() const { return ParsedBehavior.bIsValid; }

    /** Reload JSON from file (if using external file) */
    UFUNCTION(BlueprintCallable, Category = "AI Behavior")
    bool ReloadFromFile(FString& OutError);

    // ==================== Asset Interface ====================

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    /** Parse JSON string into behavior definition */
    bool ParseJsonInternal(const FString& JsonString, FAIBehaviorDef& OutDef, FString& OutError);
};
