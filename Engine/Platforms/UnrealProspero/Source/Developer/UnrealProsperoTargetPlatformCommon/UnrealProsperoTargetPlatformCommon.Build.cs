using System.IO;
using UnrealBuildTool;

public class UnrealProsperoTargetPlatformCommon : ModuleRules
{
    public UnrealProsperoTargetPlatformCommon(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "TargetPlatform", "DesktopPlatform" });
        PublicIncludePaths.Add(Path.Combine(EngineDirectory, "Platforms/UnrealProspero/Source/Runtime/Core/Public"));
        if (target.bCompileAgainstEngine) PublicDependencyModuleNames.AddRange(new[] { "Engine", "RHI" });
    }
}
