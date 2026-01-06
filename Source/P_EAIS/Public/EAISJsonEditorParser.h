// Copyright Punal Manalan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EAIS_Types.h"
#include "EAISJsonEditorParser.generated.h"

/**
 * Node data for the visual editor, stored in the "editor" section of the JSON.
 */
USTRUCT()
struct FEAIS_EditorNodeData
{
    GENERATED_BODY()

    UPROPERTY()
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY()
    bool bCollapsed = false;
};

/**
 * Viewport data for the visual editor.
 */
USTRUCT()
struct FEAIS_EditorViewportData
{
    GENERATED_BODY()

    UPROPERTY()
    FVector2D ViewOffset = FVector2D::ZeroVector;

    UPROPERTY()
    float ZoomAmount = 1.0f;
};

/**
 * Handles parsing and serialization of EAIS JSON files including editor-specific metadata.
 */
class P_EAIS_API FEAISJsonEditorParser
{
public:
    /** Parse JSON from string, including editor metadata */
    static bool LoadFromJson(const FString& JsonString, FAIBehaviorDef& OutDef, TMap<FString, FEAIS_EditorNodeData>& OutEditorNodes, FEAIS_EditorViewportData& OutViewport);

    /** Save behavior and editor metadata to JSON string */
    static bool SaveToJson(const FAIBehaviorDef& Def, const TMap<FString, FEAIS_EditorNodeData>& EditorNodes, const FEAIS_EditorViewportData& Viewport, FString& OutJsonString);

private:
    // Internal helpers for parsing canonical vs C++ fields
    static void ParseState(const TSharedPtr<FJsonObject>& StateObj, FAIState& OutState);
    static void SerializeState(const FAIState& State, TSharedRef<FJsonObject> StateObj);
};
