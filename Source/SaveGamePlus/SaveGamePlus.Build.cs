using UnrealBuildTool;

public class SaveGamePlus : ModuleRules
{
	public SaveGamePlus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
				"ApplicationCore",
				"RenderCore",
				"ImageWrapper",
				"Projects",
				"ImageWrapper",
				"OodleDataCompression"
			}
		);
	}
}
