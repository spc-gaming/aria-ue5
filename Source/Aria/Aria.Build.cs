// Copyright (c) SPC Gaming. All rights reserved.

using UnrealBuildTool;

public class Aria : ModuleRules
{
	public Aria(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Niagara"
		});
	}
}