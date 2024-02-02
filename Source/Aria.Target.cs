// Copyright (c) SPC Gaming. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AriaTarget : TargetRules
{
	public AriaTarget(TargetInfo target) : base(target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("Aria");
	}
}
