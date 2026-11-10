using System;
using AnyUnreal.Build.Sdk;

namespace UnrealBuildTool;

public partial struct UnrealTargetPlatform
{
    public static UnrealTargetPlatform AnyWindows { get; } = FindOrAddByName("AnyWindows");
    public static UnrealTargetPlatform AnyLinux { get; } = FindOrAddByName("AnyLinux");
    public static UnrealTargetPlatform AnyPS5 { get; } = FindOrAddByName("AnyPS5");
}

public partial struct UnrealPlatformGroup
{
    public static UnrealPlatformGroup AnyUnreal { get; } = FindOrAddByName("AnyUnreal");
}

internal static class AnyUnrealPlatformNames
{
    public static Target GetTarget(UnrealTargetPlatform platform)
    {
        if (platform == UnrealTargetPlatform.AnyWindows) return Target.Windows;
        if (platform == UnrealTargetPlatform.AnyLinux) return Target.Linux;
        if (platform == UnrealTargetPlatform.AnyPS5) return Target.Ps5;
        throw new ArgumentOutOfRangeException(nameof(platform), platform, "Not an AnyUnreal platform");
    }
}
