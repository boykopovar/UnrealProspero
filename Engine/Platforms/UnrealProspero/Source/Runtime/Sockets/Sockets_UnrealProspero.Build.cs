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
        PrivateDefinitions.Add("UE_SOCKET_API_WRAPPER=\"UnrealProspero/SocketApiWrapper.h\"");
        PrivateDefinitions.Add("PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL=1");
        PrivateDefinitions.Add("PLATFORM_HAS_BSD_SOCKET_FEATURE_POLL=1");
        PublicSystemLibraries.Add("libScePosixForWebKit");
    }
}
