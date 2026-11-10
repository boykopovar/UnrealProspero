using Microsoft.Extensions.Logging;

namespace UnrealBuildTool;

internal abstract class AnyUnrealPlatformFactory : UEBuildPlatformFactory
{
    public override void RegisterBuildPlatforms(ILogger logger)
    {
        UEBuildPlatform.RegisterBuildPlatform(new AnyUnrealPlatform(TargetPlatform, AnyUnrealSettings.Load(), logger), logger);
        UEBuildPlatform.RegisterPlatformWithGroup(TargetPlatform, UnrealPlatformGroup.AnyUnreal);
    }
}

internal sealed class AnyWin64PlatformFactory : AnyUnrealPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyWin64;
}

internal sealed class AnyLinuxPlatformFactory : AnyUnrealPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyLinux;
}

internal sealed class AnyPS5PlatformFactory : AnyUnrealPlatformFactory
{
    public override UnrealTargetPlatform TargetPlatform => UnrealTargetPlatform.AnyPS5;
}
