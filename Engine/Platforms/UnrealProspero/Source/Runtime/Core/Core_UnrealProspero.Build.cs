using UnrealBuildTool;

public sealed class Core_UnrealProspero : Core
{
    public Core_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.AddRange(new[] { "libkernel_sys", "libSceUserService", "libSceSysmodule" });
        if (target.Platform == UnrealTargetPlatform.AnyPS5) PublicSystemLibraries.Add("libSceSystemService");
        AddEngineThirdPartyPrivateStaticDependencies(target, "zlib");
    }
}
