using System;
using System.Collections.Generic;
using UnrealBuildTool;

internal static class Program
{
    private static void Main()
    {
        UnrealTargetPlatform[] platforms = { UnrealTargetPlatform.AnyWindows, UnrealTargetPlatform.AnyLinux, UnrealTargetPlatform.AnyPS5 };
        if (new HashSet<UnrealTargetPlatform>(platforms).Count != platforms.Length) throw new InvalidOperationException("Platform identities collide");
        foreach (UnrealTargetPlatform platform in platforms)
        {
            if (!UnrealTargetPlatform.TryParse(platform.ToString(), out UnrealTargetPlatform parsed) || platform != parsed) throw new InvalidOperationException("Platform name does not round-trip");
            if (platform == UnrealTargetPlatform.Win64 || platform == UnrealTargetPlatform.Linux) throw new InvalidOperationException("Extension replaces a native platform");
            DataDrivenPlatformInfo.ConfigDataDrivenPlatformInfo info = DataDrivenPlatformInfo.GetAllPlatformInfos()[platform.ToString()];
            if (!info.bIsEnabled || info.bIsConfidential || info.bCanUseCrashReporter) throw new InvalidOperationException("Invalid platform configuration");
            if (info.IniParentChain == null || info.IniParentChain.Length != 1 || info.IniParentChain[0] != "AnyUnreal") throw new InvalidOperationException("Shared configuration is not inherited");
            Console.WriteLine("Discovered " + platform);
        }
        if (UnrealPlatformGroup.AnyUnreal.ToString() != "AnyUnreal") throw new InvalidOperationException("Invalid platform group");
    }
}
