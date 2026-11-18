using System.IO;
using UnrealBuildTool;

public sealed class Sockets_UnrealProspero : Sockets
{
    public Sockets_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyPS5)
        {
            return;
        }

        bEnableExceptions = true;
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PrivateIncludePaths.Add(Path.Combine(PlatformModuleDirectory, "Private"));
        PublicSystemLibraries.Add("libScePosixForWebKit");
    }
}
