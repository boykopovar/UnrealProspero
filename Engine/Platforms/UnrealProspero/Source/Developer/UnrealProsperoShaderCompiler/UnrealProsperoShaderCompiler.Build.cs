using UnrealBuildTool;

public class UnrealProsperoShaderCompiler : ModuleRules
{
    public UnrealProsperoShaderCompiler(ReadOnlyTargetRules target) : base(target)
    {
        if (target.bBuildRequiresCookedData) throw new BuildException("UnrealProspero shader compilation is only available during cook and build");
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "UnrealProsperoShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "ShaderCompilerCommon" });
        AddEngineThirdPartyPrivateStaticDependencies(target, "SPIRVReflect");
    }
}
