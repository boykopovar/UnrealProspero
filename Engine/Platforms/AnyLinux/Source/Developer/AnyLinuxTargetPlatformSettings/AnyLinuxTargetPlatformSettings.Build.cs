using UnrealBuildTool;

public class AnyLinuxTargetPlatformSettings : ModuleRules
{
    public AnyLinuxTargetPlatformSettings(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "AnyUnrealTargetPlatformCommon" });
    }
}
