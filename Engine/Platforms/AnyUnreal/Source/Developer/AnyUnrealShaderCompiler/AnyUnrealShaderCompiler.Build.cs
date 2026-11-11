using UnrealBuildTool;

public class AnyUnrealShaderCompiler : ModuleRules
{
    public AnyUnrealShaderCompiler(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "RenderCore", "RHI", "ShaderCompilerCommon" });
        AddEngineThirdPartyPrivateStaticDependencies(target, "SPIRVReflect");
    }
}
