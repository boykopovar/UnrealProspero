using UnrealBuildTool;

public class AnyUnrealShaderCompiler : ModuleRules
{
    public AnyUnrealShaderCompiler(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.Add("Core");
        AddEngineThirdPartyPrivateStaticDependencies(target, "SPIRVReflect");
    }
}
