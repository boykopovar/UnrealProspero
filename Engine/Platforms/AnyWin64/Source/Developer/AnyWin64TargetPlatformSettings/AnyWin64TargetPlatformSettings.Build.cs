using UnrealBuildTool;

public class AnyWin64TargetPlatformSettings : ModuleRules
{
    public AnyWin64TargetPlatformSettings(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "AnyUnrealTargetPlatformCommon" });
    }
}
