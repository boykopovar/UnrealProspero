using Microsoft.Extensions.Logging;

namespace UnrealBuildTool;

internal abstract class UnrealProsperoPlatformFactory : UEBuildPlatformFactory
{
    public override void RegisterBuildPlatforms(ILogger logger)
    {
        UEBuildPlatform.RegisterBuildPlatform(new UnrealProsperoPlatform(TargetPlatform, UnrealProsperoSettings.Load(), logger), logger);
        UEBuildPlatform.RegisterPlatformWithGroup(TargetPlatform, UnrealPlatformGroup.UnrealProspero);
    }
}

internal sealed class AnyWin64PlatformFactory : UnrealProsperoPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyWin64;
}

internal sealed class AnyLinuxPlatformFactory : UnrealProsperoPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyLinux;
}

internal sealed class AnyPS5PlatformFactory : UnrealProsperoPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyPS5;
}
