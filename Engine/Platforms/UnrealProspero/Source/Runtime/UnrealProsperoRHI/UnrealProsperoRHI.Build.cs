using UnrealBuildTool;

public class UnrealProsperoRHI : ModuleRules
{
    public UnrealProsperoRHI(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDefinitions.Add("UNREALPROSPERO_WITH_UE_TRACKING=1");
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "ApplicationCore", "UnrealProsperoShaderCommon" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "RHICore" });
        PublicSystemLibraries.Add("libSceAgc");
        PublicSystemLibraries.Add("libSceAgcDriver");
    }
}
