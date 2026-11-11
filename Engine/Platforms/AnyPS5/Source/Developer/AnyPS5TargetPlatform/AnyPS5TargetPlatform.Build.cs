using UnrealBuildTool;

public class AnyPS5TargetPlatform : ModuleRules
{
    public AnyPS5TargetPlatform(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "UnrealProsperoTargetPlatformCommon" });
    }
}
