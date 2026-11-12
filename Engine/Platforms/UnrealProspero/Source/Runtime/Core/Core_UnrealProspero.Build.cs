using UnrealBuildTool;

public sealed class Core_UnrealProspero : Core
{
    public Core_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.Add("libkernel_sys");
        if (target.Platform == UnrealTargetPlatform.AnyPS5) PublicSystemLibraries.Add("libSceSystemService");
        AddEngineThirdPartyPrivateStaticDependencies(target, "zlib");
    }
}
