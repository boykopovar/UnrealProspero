using System.IO;
using UnrealBuildTool;

public class AnyUnrealTargetPlatformCommon : ModuleRules
{
    public AnyUnrealTargetPlatformCommon(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "DesktopPlatform" });
        PublicIncludePaths.Add(Path.Combine(EngineDirectory, "Platforms/AnyUnreal/Source/Runtime/Core/Public"));
        if (target.bCompileAgainstEngine) PublicDependencyModuleNames.AddRange(new[] { "Engine", "RHI" });
    }
}
