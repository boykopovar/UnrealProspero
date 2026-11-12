using UnrealBuildTool;

public sealed class Core_UnrealProspero : Core
{
    public Core_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.Add("libkernel_sys");
        AddEngineThirdPartyPrivateStaticDependencies(target, "zlib");
    }
}
