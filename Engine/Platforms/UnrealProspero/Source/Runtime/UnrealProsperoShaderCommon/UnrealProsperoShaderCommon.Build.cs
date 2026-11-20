using UnrealBuildTool;

public class UnrealProsperoShaderCommon : ModuleRules
{
    public UnrealProsperoShaderCommon(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.Add("RHI");
        PrivateDependencyModuleNames.Add("Core");
    }
}
