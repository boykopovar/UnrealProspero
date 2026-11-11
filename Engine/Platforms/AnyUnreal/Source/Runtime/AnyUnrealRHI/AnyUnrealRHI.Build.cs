using UnrealBuildTool;

public class AnyUnrealRHI : ModuleRules
{
    public AnyUnrealRHI(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "AnyUnrealShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RHI", "RenderCore" });
        PublicSystemLibraries.Add("libSceAgc");
    }
}
