// UNVERIFIED IN THIS ENVIRONMENT.
using UnrealBuildTool;
using System.Collections.Generic;

public class EarthFrontierEditorTarget : TargetRules
{
	public EarthFrontierEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.AddRange(new string[] { "EFWorldCore", "EarthFrontier" });
	}
}
