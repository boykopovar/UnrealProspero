using System.IO;
using UnrealBuildTool;

public sealed class IoStoreHttpClient_UnrealProspero : IoStoreHttpClient
{
    public IoStoreHttpClient_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyPS5) return;
        PrivateIncludePaths.Add(Path.Combine(PlatformModuleDirectory, "Private"));
        PublicSystemLibraries.Add("libScePosixForWebKit");
    }
}
