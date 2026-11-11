using UnrealBuildTool;

public class AnyLinuxTargetPlatform : ModuleRules
{
    public AnyLinuxTargetPlatform(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "AnyUnrealTargetPlatformCommon" });
    }
}
