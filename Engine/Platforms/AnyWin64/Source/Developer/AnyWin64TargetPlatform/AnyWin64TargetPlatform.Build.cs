using UnrealBuildTool;

public class AnyWin64TargetPlatform : ModuleRules
{
    public AnyWin64TargetPlatform(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "AnyUnrealTargetPlatformCommon" });
    }
}
