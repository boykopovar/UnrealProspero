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
        if (target.Platform == UnrealTargetPlatform.Win64) PrivateIncludePathModuleNames.Add("WindowsTargetPlatformControls");
        else if (target.Platform == UnrealTargetPlatform.Linux) PrivateIncludePathModuleNames.Add("LinuxTargetPlatformControls");
        if (target.bCompileAgainstEngine) PublicDependencyModuleNames.AddRange(new[] { "Engine", "RHI" });
    }
}
