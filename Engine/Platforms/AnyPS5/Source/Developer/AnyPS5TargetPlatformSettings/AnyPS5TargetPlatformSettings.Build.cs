using UnrealBuildTool;

public class AnyPS5TargetPlatformSettings : ModuleRules
{
    public AnyPS5TargetPlatformSettings(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "UnrealProsperoTargetPlatformCommon" });
        if (target.bCompileAgainstEngine) PrivateDependencyModuleNames.Add("Engine");
    }
}
