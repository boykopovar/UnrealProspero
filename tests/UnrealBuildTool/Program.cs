using System;
using System.Collections.Generic;
using AnyUnreal.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging.Abstractions;
using UnrealBuildTool;

internal static class Program
{
    private static void Main(string[] args)
    {
        if (args.Length != 3) throw new ArgumentException("Expected SDK, dependencies and -rootdirectory=<unreal checkout>");
        SdkInstallation installation = new(args[0], args[1]);
        UnrealTargetPlatform[] platforms = { UnrealTargetPlatform.AnyWindows, UnrealTargetPlatform.AnyLinux, UnrealTargetPlatform.AnyPS5 };
        Target[] targets = { Target.Windows, Target.Linux, Target.Ps5 };
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
        for (int index = 0; index < platforms.Length; index++)
        {
            AnyUnrealPlatformSdk sdk = new(NullLogger.Instance, targets[index], installation);
            UEBuildPlatformSDK.RegisterSDKForPlatform(sdk, platforms[index].ToString(), true);
            if (sdk.HasRequiredManualSDK() != SDKStatus.Valid) throw new InvalidOperationException("Installed SDK was rejected: " + platforms[index]);
            if (!sdk.TryConvertVersionToInt(installation.CxxVersion, out ulong installed, null) || !sdk.TryConvertVersionToInt(sdk.GetMainVersion(), out ulong expected, null) || installed != expected) throw new InvalidOperationException("SDK version mismatch");
            foreach (string invalid in new[] { "", "v0.1", "v0.1.9.0", "v0.1.9-preview", "v65536.0.0" })
            {
                if (sdk.TryConvertVersionToInt(invalid, out _, null)) throw new InvalidOperationException("Invalid SDK version accepted: " + invalid);
            }
            Console.WriteLine("Validated Unreal SDK for " + platforms[index]);
        }
    }
}
