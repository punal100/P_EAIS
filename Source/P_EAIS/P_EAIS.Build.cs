using UnrealBuildTool;

public class P_EAIS : ModuleRules
{
	public P_EAIS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// Add public include paths here
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				// Add private include paths here
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"NavigationSystem",
				"AIModule",          // For AAIController, PathFollowingComponent
				"P_MEIS",            // For input injection
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Json",
				"JsonUtilities",
				"Slate",
				"SlateCore",
			}
		);

		// Allow circular dependency with P_MiniFootball for AI integration
		// CircularlyReferencedDependentModules.Add("P_MiniFootball");
	}
}
