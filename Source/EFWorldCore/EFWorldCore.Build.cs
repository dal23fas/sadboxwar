// UNVERIFIED IN THIS ENVIRONMENT (no UnrealBuildTool available). Compiles the same engine-neutral
// sources that CMake builds and tests (Source/EFWorldCore/Private/*.cpp) as a UE runtime module.
using UnrealBuildTool;

public class EFWorldCore : ModuleRules
{
	public EFWorldCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// Pure standard C++17, no UE headers, no PCH, no exceptions, no RTTI.
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp17;
		bEnableExceptions = false;
		bUseRTTI = false;
		bUseUnity = false;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PrivateIncludePaths.Add(ModuleDirectory + "/Private");

		// "Core" is required by UBT for every module even though no UE symbols are used here.
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });
	}
}
