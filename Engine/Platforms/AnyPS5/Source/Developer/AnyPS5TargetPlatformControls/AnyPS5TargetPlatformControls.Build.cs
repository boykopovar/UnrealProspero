using UnrealBuildTool;

public class AnyPS5TargetPlatformControls : ModuleRules
{
    public AnyPS5TargetPlatformControls(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "UnrealProsperoTargetPlatformCommon", "AnyPS5TargetPlatformSettings" });
    }
}
