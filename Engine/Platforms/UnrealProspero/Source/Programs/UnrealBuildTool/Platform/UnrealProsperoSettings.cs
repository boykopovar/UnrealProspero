using System;
using System.IO;
using UnrealProspero.Build.Sdk;

namespace UnrealBuildTool;

public sealed class UnrealProsperoSettings
{
    [XmlConfigFile(Category = "UnrealProspero")]
    public string? SdkRoot { get; set; }

    [XmlConfigFile(Category = "UnrealProspero")]
    public string? Dependencies { get; set; }

    [XmlConfigFile(Category = "UnrealProspero")]
    public string? CMake { get; set; }

    [XmlConfigFile(Category = "UnrealProspero")]
    public int Ps5ShaderPlatformIndex { get; set; } = 47;

    public bool IsConfigured => SdkRoot != null || Dependencies != null || CMake != null;

    public static UnrealProsperoSettings Load()
    {
        UnrealProsperoSettings settings = new();
        XmlConfig.ApplyTo(settings);
        return settings;
    }

    internal UEBuildPlatform.StaticShader GetShaderPlatform(Target target)
    {
        if (Ps5ShaderPlatformIndex < 32 || Ps5ShaderPlatformIndex > 47) throw new InvalidOperationException("UnrealProspero shader index must use UE's static platform range 32..47");
        return target switch
        {
            Target.Ps5 => new() { Name = "AGC_ANYPS5", Index = Ps5ShaderPlatformIndex },
            _ => throw new ArgumentOutOfRangeException(nameof(target))
        };
    }

    public SdkInstallation CreateInstallation()
    {
        if (string.IsNullOrWhiteSpace(SdkRoot) || string.IsNullOrWhiteSpace(Dependencies)) throw new InvalidOperationException("UnrealProspero requires SdkRoot and Dependencies in BuildConfiguration.xml");
        RequireCMake();
        return new SdkInstallation(SdkRoot, Dependencies);
    }

    public string RequireCMake()
    {
        if (string.IsNullOrWhiteSpace(CMake)) throw new InvalidOperationException("UnrealProspero requires the CMake executable path in BuildConfiguration.xml");
        return SdkInstallation.RequireFile(Path.GetFullPath(CMake));
    }
}
