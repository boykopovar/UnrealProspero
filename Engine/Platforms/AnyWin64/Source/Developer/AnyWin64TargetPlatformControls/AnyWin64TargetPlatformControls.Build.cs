using UnrealBuildTool;

public class AnyWin64TargetPlatformControls : ModuleRules
{
    public AnyWin64TargetPlatformControls(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "DesktopPlatform", "TargetPlatform", "UnrealProsperoTargetPlatformCommon", "AnyWin64TargetPlatformSettings" });
        if (target.bCompileAgainstEngine) PrivateDependencyModuleNames.Add("Engine");
    }
}
