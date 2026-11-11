using UnrealBuildTool;

public class AnyUnrealRHI : ModuleRules
{
    public AnyUnrealRHI(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "ApplicationCore", "AnyUnrealShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "RHICore" });
        PublicSystemLibraries.Add("libSceAgc");
        PublicSystemLibraries.Add("libSceAgcDriver");
    }
}
