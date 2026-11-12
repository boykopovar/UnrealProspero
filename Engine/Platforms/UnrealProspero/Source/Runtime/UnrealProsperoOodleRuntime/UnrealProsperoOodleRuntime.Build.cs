using UnrealBuildTool;

public sealed class UnrealProsperoOodleRuntime : ModuleRules
{
    public UnrealProsperoOodleRuntime(ReadOnlyTargetRules target) : base(target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
    }
}
