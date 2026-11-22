using System;
using System.IO;
using System.Text.Json;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;

namespace UnrealBuildTool;

public sealed class UnrealProsperoPlatformSdk : UEBuildPlatformSDK
{
    private readonly UnrealProsperoSettings settings;
    private SdkInstallation? installation;
    private readonly Target target;

    public UnrealProsperoPlatformSdk(ILogger logger, Target target, UnrealProsperoSettings settings) : base(logger)
    {
        SdkInstallation.GetTargetName(target);
        this.target = target;
        this.settings = settings;
    }

    public SdkInstallation RequireInstallation()
    {
        if (!settings.IsConfigured) throw new InvalidOperationException("Configure the UnrealProspero SDK and dependency directories before building");
        installation ??= settings.CreateInstallation();
        installation.ValidateTarget(target);
        return installation;
    }

    protected override string? GetInstalledSDKVersion()
    {
        if (settings.Dependencies == null) return null;
        string metadata = Path.Combine(settings.Dependencies, "dependencies.json");
        if (!File.Exists(metadata)) return null;
        try
        {
            using JsonDocument document = JsonDocument.Parse(File.ReadAllText(metadata));
            JsonElement root = document.RootElement;
            if (root.ValueKind != JsonValueKind.Object) return null;
            if (!root.TryGetProperty("schemaVersion", out JsonElement schema) || schema.ValueKind != JsonValueKind.Number || !schema.TryGetInt32(out int version) || version != 1) return null;
            if (!root.TryGetProperty("anycxx", out JsonElement cxx) || cxx.ValueKind != JsonValueKind.String) return null;
            if (!root.TryGetProperty("targets", out JsonElement targets) || targets.ValueKind != JsonValueKind.Array) return null;
            foreach (JsonElement item in targets.EnumerateArray())
            {
                if (item.ValueKind == JsonValueKind.String && item.GetString() == SdkInstallation.GetTargetName(target)) return cxx.GetString();
            }
            return null;
        }
        catch (IOException)
        {
            return null;
        }
        catch (UnauthorizedAccessException)
        {
            return null;
        }
        catch (JsonException)
        {
            return null;
        }
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
