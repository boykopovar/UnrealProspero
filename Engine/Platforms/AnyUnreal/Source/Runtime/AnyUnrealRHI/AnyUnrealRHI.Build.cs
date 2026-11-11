using UnrealBuildTool;

public class AnyUnrealRHI : ModuleRules
{
    public AnyUnrealRHI(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "RHI", "AnyUnrealShaderCommon" });
        PrivateDependencyModuleNames.Add("RenderCore");
        PublicSystemLibraries.Add("libSceAgc");
    }
}
