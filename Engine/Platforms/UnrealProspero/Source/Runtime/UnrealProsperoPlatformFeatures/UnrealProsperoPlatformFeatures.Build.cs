using UnrealBuildTool;

public sealed class UnrealProsperoPlatformFeatures : ModuleRules
{
    public UnrealProsperoPlatformFeatures(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "Engine" });
    }
}
