/*
 * @Author: Punal Manalan
 * @Description: EAIS Editor Widget Spec for P_MWCS generation
 * @Date: 29/12/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EAIS_EditorWidgetSpec.generated.h"

/**
 * Widget specification for the EAIS Editor Tool.
 * Used by P_MWCS to generate the editor UI.
 */
UCLASS()
class P_EAISTOOLS_API UEAIS_EditorWidgetSpec : public UObject
{
    GENERATED_BODY()

public:
    /** Get the widget spec as JSON string */
    UFUNCTION(BlueprintCallable, Category = "EAIS Editor")
    static FString GetWidgetSpec();

    /** Get the widget class name for P_MWCS */
    UFUNCTION(BlueprintPure, Category = "EAIS Editor")
    static FString GetWidgetClassName() { return TEXT("EUW_EAIS_AIEditor"); }
};
