using System;
using UnrealProspero.Build.Sdk;

namespace UnrealBuildTool;

public partial struct UnrealTargetPlatform
{
    public static UnrealTargetPlatform AnyWin64 { get; } = FindOrAddByName("AnyWin64");
    public static UnrealTargetPlatform AnyLinux { get; } = FindOrAddByName("AnyLinux");
    public static UnrealTargetPlatform AnyPS5 { get; } = FindOrAddByName("AnyPS5");
}

public partial struct UnrealPlatformGroup
{
    public static UnrealPlatformGroup UnrealProspero { get; } = FindOrAddByName("UnrealProspero");
}

internal static class UnrealProsperoPlatformNames
{
    public static Target GetTarget(UnrealTargetPlatform platform)
    {
        if (platform == UnrealTargetPlatform.AnyWin64) return Target.Windows;
        if (platform == UnrealTargetPlatform.AnyLinux) return Target.Linux;
        if (platform == UnrealTargetPlatform.AnyPS5) return Target.Ps5;
        throw new ArgumentOutOfRangeException(nameof(platform), platform, "Not an UnrealProspero platform");
    }
}
