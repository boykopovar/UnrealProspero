using UnrealBuildTool;

public class AnyUnrealShaderCompiler : ModuleRules
{
    public AnyUnrealShaderCompiler(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "AnyUnrealShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "ShaderCompilerCommon" });
        AddEngineThirdPartyPrivateStaticDependencies(target, "SPIRVReflect");
    }
}
