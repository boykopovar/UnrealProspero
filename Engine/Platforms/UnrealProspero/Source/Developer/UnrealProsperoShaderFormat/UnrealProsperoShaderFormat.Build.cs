using System;
using UnrealBuildTool;

public class UnrealProsperoShaderFormat : ModuleRules
{
    public UnrealProsperoShaderFormat(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "ShaderCompilerCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealProsperoShaderCompiler", "UnrealProsperoShaderCommon", "RenderCore", "TargetPlatform" });
        string executable = UnrealProsperoSettings.Load().CreateInstallation().Executable.Replace('\\', '/');
        if (executable.Contains('"') || executable.Contains('\n') || executable.Contains('\r')) throw new InvalidOperationException("Invalid ADK executable path");
        PrivateDefinitions.Add("UNREALPROSPERO_ADK_EXECUTABLE=\"" + executable + "\"");
        ExternalDependencies.Add(executable);
    }
}
