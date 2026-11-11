using UnrealBuildTool;

public class UnrealProsperoShaderCompiler : ModuleRules
{
    public UnrealProsperoShaderCompiler(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "UnrealProsperoShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "ShaderCompilerCommon" });
        AddEngineThirdPartyPrivateStaticDependencies(target, "SPIRVReflect");
    }
}
