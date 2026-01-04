// Copyright Punal Manalan. All Rights Reserved.
// Editor-only JSON parser for EAIS

#pragma once

#include "CoreMinimal.h"
#include "EAIS_Types.h"

/**
 * Editor-only representation of an AI graph.
 * Contains runtime-like data plus editor layout metadata.
 */
struct FAIEditorGraph
{
    struct FEditorState
    {
        FString Id;
        bool bTerminal = false;
        TArray<FAIActionEntry> OnEnter;
        TArray<FAIActionEntry> OnTick;
        TArray<FAIActionEntry> OnExit;
        TArray<FAITransition> Transitions;
    };

    FString Name;
    FString InitialState;
    TArray<FEditorState> States;
    
    // Editor-only metadata (positions, colors, etc.)
    TSharedPtr<class FJsonObject> EditorMetadata;
};

/**
 * Editor-only JSON parser.
 * MUST be used instead of FAIInterpreter for all editor operations.
 */
class P_EAIS_EDITOR_API FEAISJsonEditorParser
{
public:
    /**
     * Parse editor JSON (includes layout/editor metadata) into editor graph.
     * @param Json Raw JSON string
     * @param OutGraph Parsed editor graph
     * @param OutError Error message if parsing fails
     * @return true if parsing succeeded
     */
    static bool ParseEditorJson(const FString& Json, FAIEditorGraph& OutGraph, FString& OutError);

    /**
     * Convert editor graph to canonical runtime BehaviorDef (strips editor metadata).
     * @param InGraph Editor graph to convert
     * @param OutDef Runtime behavior definition
     * @param OutError Error message if conversion fails
     * @return true if conversion succeeded
     */
    static bool ConvertEditorGraphToRuntime(const FAIEditorGraph& InGraph, FAIBehaviorDef& OutDef, FString& OutError);
    
    /**
     * Serialize editor graph to editor JSON (includes metadata).
     * @param InGraph Editor graph
     * @return JSON string
     */
    static FString SerializeEditorGraph(const FAIEditorGraph& InGraph);
};

/**
 * Runtime JSON serializer.
 * Produces deterministic, canonical JSON output.
 */
class P_EAIS_EDITOR_API FEAISJsonSerializer
{
public:
    /**
     * Serialize runtime behavior definition to JSON.
     * @param InDef Behavior definition
     * @return Canonical JSON string (no editor metadata)
     */
    static FString SerializeRuntime(const FAIBehaviorDef& InDef);
};
