using UnrealBuildTool;

public class UnrealProsperoShaderCooker : ModuleRules
{
    public UnrealProsperoShaderCooker(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Type != TargetType.Editor) throw new BuildException("UnrealProspero shader finalization requires an editor target");
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.Add("UnrealProsperoShaderCompiler");
    }
}
