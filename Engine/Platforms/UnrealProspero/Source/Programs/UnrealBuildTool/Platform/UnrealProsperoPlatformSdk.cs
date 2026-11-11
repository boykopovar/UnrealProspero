using System;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;

namespace UnrealBuildTool;

public sealed class UnrealProsperoPlatformSdk : UEBuildPlatformSDK
{
    private readonly SdkInstallation? installation;
    private readonly Target target;

    public UnrealProsperoPlatformSdk(ILogger logger, Target target, SdkInstallation? installation) : base(logger)
    {
        SdkInstallation.GetTargetName(target);
        this.target = target;
        this.installation = installation;
    }

    public SdkInstallation RequireInstallation()
    {
        if (installation == null) throw new InvalidOperationException("Configure the UnrealProspero SDK and dependency directories before building");
        installation.ValidateTarget(target);
        return installation;
    }

    protected override string? GetInstalledSDKVersion()
    {
        if (installation == null) return null;
        return RequireInstallation().CxxVersion;
    }

    public override bool TryConvertVersionToInt(string? value, out ulong result, string? hint)
    {
        result = 0;
        if (string.IsNullOrEmpty(value)) return false;
        string versionText = value[0] == 'v' ? value.Substring(1) : value;
        if (!Version.TryParse(versionText, out Version? version)) return false;
        if (version.Major > ushort.MaxValue || version.Minor > ushort.MaxValue || version.Build < 0 || version.Build > ushort.MaxValue || version.Revision != -1) return false;
        result = ((ulong)version.Major << 32) | ((ulong)version.Minor << 16) | (uint)version.Build;
        return true;
    }
}
