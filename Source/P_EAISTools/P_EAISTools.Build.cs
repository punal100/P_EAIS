using UnrealBuildTool;

public class P_EAISTools : ModuleRules
{
	public P_EAISTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"P_EAIS",
				"P_MWCS",          // For Editor widget generation
				"UMG",
				"UnrealEd",
				"Blutility",
				"UMGEditor",
				"LevelEditor",     // For menu extensions
				"EditorStyle",
				"PropertyEditor",
				"Json",
				"JsonUtilities",
			}
		);
	}
}
