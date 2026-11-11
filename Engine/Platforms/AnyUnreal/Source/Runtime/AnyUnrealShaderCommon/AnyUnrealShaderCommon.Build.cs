using UnrealBuildTool;

public class AnyUnrealShaderCommon : ModuleRules
{
    public AnyUnrealShaderCommon(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PrivateDependencyModuleNames.Add("Core");
    }
}
