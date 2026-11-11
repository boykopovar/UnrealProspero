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
    public int WindowsShaderPlatformIndex { get; set; } = 45;

    [XmlConfigFile(Category = "UnrealProspero")]
    public int LinuxShaderPlatformIndex { get; set; } = 46;

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
        foreach (int index in new[] { WindowsShaderPlatformIndex, LinuxShaderPlatformIndex, Ps5ShaderPlatformIndex })
        {
            if (index < 32 || index > 47) throw new InvalidOperationException("UnrealProspero shader indices must use UE's static platform range 32..47");
        }
        if (WindowsShaderPlatformIndex == LinuxShaderPlatformIndex || WindowsShaderPlatformIndex == Ps5ShaderPlatformIndex || LinuxShaderPlatformIndex == Ps5ShaderPlatformIndex) throw new InvalidOperationException("UnrealProspero shader platform indices must be distinct");
        return target switch
        {
            Target.Windows => new() { Name = "AGC_ANYWIN64", Index = WindowsShaderPlatformIndex },
            Target.Linux => new() { Name = "AGC_ANYLINUX", Index = LinuxShaderPlatformIndex },
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
