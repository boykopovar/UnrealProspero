using UnrealBuildTool;

public class AnyLinuxTargetPlatformControls : ModuleRules
{
    public AnyLinuxTargetPlatformControls(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "DesktopPlatform", "TargetPlatform", "UnrealProsperoTargetPlatformCommon", "AnyLinuxTargetPlatformSettings" });
    }
}
