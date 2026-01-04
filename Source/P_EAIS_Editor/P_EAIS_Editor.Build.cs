// Copyright Punal Manalan. All Rights Reserved.

using UnrealBuildTool;

public class P_EAIS_Editor : ModuleRules
{
    public P_EAIS_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "P_EAIS"
        });
        
        PrivateDependencyModuleNames.AddRange(new string[] 
        { 
            "UnrealEd",
            "Slate",
            "SlateCore",
            "GraphEditor",
            "EditorStyle",
            "Json",
            "JsonUtilities",
            "InputCore",
            "ToolMenus",
            "EditorFramework",
            "ApplicationCore",
            "PropertyEditor"
        });
    }
}

