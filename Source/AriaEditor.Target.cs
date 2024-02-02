// Copyright (c) SPC Gaming. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AriaEditorTarget : TargetRules
{
	public AriaEditorTarget(TargetInfo target) : base(target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("Aria");
	}
}
