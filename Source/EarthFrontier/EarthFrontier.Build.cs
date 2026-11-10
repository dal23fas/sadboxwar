// UNVERIFIED IN THIS ENVIRONMENT. Game module: presentation + input + UE subsystem wrapping EFWorldCore.
using UnrealBuildTool;

public class EarthFrontier : ModuleRules
{
	public EarthFrontier(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "EFWorldCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });
	}
}
