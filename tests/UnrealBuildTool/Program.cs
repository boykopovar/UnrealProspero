using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging.Abstractions;
using UnrealBuildTool;

internal static class Program
{
    private static async Task Main(string[] args)
    {
        if (args.Length < 5 || args.Length > 6 || (args.Length == 6 && args[5] != "--rhi-only")) throw new ArgumentException("Expected SDK, dependencies, CMake, output directory, -rootdirectory=<unreal checkout> and optional --rhi-only");
        if (Directory.Exists(args[3]) || File.Exists(args[3])) throw new IOException("Test output already exists");
        SdkInstallation installation = new(args[0], args[1]);
        List<ReceiptProperty> receipt = new();
        UnrealProsperoReceipt.Write(receipt, installation);
        SdkInstallation recorded = UnrealProsperoReceipt.Read(receipt);
        if (recorded.Root != installation.Root || recorded.Dependencies != installation.Dependencies) throw new InvalidDataException("SDK receipt does not round-trip");
        receipt.Add(receipt[0]);
        try
        {
            UnrealProsperoReceipt.Read(receipt);
            throw new InvalidOperationException("Duplicate SDK receipt metadata was accepted");
        }
        catch (InvalidDataException)
        {
        }
        UnrealProsperoSettings settings = new() { SdkRoot = args[0], Dependencies = args[1], CMake = args[2] };
        UnrealTargetPlatform[] platforms = { UnrealTargetPlatform.AnyWin64, UnrealTargetPlatform.AnyLinux, UnrealTargetPlatform.AnyPS5 };
        Target[] targets = { Target.Windows, Target.Linux, Target.Ps5 };
        if (new HashSet<UnrealTargetPlatform>(platforms).Count != platforms.Length) throw new InvalidOperationException("Platform identities collide");
        foreach (UnrealTargetPlatform platform in platforms)
        {
            if (!UnrealTargetPlatform.TryParse(platform.ToString(), out UnrealTargetPlatform parsed) || platform != parsed) throw new InvalidOperationException("Platform name does not round-trip");
            if (platform == UnrealTargetPlatform.Win64 || platform == UnrealTargetPlatform.Linux) throw new InvalidOperationException("Extension replaces a native platform");
            DataDrivenPlatformInfo.ConfigDataDrivenPlatformInfo info = DataDrivenPlatformInfo.GetAllPlatformInfos()[platform.ToString()];
            if (!info.bIsEnabled || info.bIsConfidential || info.bCanUseCrashReporter) throw new InvalidOperationException("Invalid platform configuration");
            if (info.IniParentChain == null || info.IniParentChain.Length != 1 || info.IniParentChain[0] != "UnrealProspero") throw new InvalidOperationException("Shared configuration is not inherited");
            Console.WriteLine("Discovered " + platform);
        }
        if (UnrealPlatformGroup.UnrealProspero.ToString() != "UnrealProspero") throw new InvalidOperationException("Invalid platform group");
        for (int index = 0; index < platforms.Length; index++)
        {
            UnrealProsperoPlatform buildPlatform = new(platforms[index], settings, NullLogger.Instance);
            UEBuildPlatform.RegisterBuildPlatform(buildPlatform, NullLogger.Instance);
            UEBuildPlatform.RegisterPlatformWithGroup(platforms[index], UnrealPlatformGroup.UnrealProspero);
            UnrealProsperoPlatformSdk sdk = (UnrealProsperoPlatformSdk)buildPlatform.GetSDK()!;
            if (sdk.HasRequiredManualSDK() != SDKStatus.Valid) throw new InvalidOperationException("Installed SDK was rejected: " + platforms[index]);
            if (!sdk.TryConvertVersionToInt(installation.CxxVersion, out ulong installed, null) || !sdk.TryConvertVersionToInt(sdk.GetMainVersion(), out ulong expected, null) || installed != expected) throw new InvalidOperationException("SDK version mismatch");
            foreach (string invalid in new[] { "", "v0.1", "v0.1.9.0", "v0.1.9-preview", "v65536.0.0" })
            {
                if (sdk.TryConvertVersionToInt(invalid, out _, null)) throw new InvalidOperationException("Invalid SDK version accepted: " + invalid);
            }
            Console.WriteLine("Validated Unreal SDK for " + platforms[index]);
            await ToolchainChecks.RunAsync(installation, targets[index], platforms[index], args[2], Path.Combine(Path.GetFullPath(args[3]), platforms[index].ToString()), args.Length == 6);
        }
    }
}
