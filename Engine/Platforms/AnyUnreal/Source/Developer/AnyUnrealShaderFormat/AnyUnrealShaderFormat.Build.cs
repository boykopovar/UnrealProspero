using System;
using UnrealBuildTool;

public class AnyUnrealShaderFormat : ModuleRules
{
    public AnyUnrealShaderFormat(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "ShaderCompilerCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "AnyUnrealShaderCompiler", "AnyUnrealShaderCommon", "RenderCore", "TargetPlatform" });
        string executable = AnyUnrealSettings.Load().CreateInstallation().Executable.Replace('\\', '/');
        if (executable.Contains('"') || executable.Contains('\n') || executable.Contains('\r')) throw new InvalidOperationException("Invalid ADK executable path");
        PrivateDefinitions.Add("ANYUNREAL_ADK_EXECUTABLE=\"" + executable + "\"");
        ExternalDependencies.Add(executable);
    }
}
