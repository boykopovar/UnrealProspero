using UnrealBuildTool;

public class AnyUnrealRHI : ModuleRules
{
    public AnyUnrealRHI(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDefinitions.Add("ANYUNREAL_WITH_UE_TRACKING=1");
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "ApplicationCore", "AnyUnrealShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "RHICore" });
        PublicSystemLibraries.Add("libSceAgc");
        PublicSystemLibraries.Add("libSceAgcDriver");
    }
}
