using System;
using System.IO;
using AnyUnreal.Build.Sdk;

namespace UnrealBuildTool;

public sealed class AnyUnrealSettings
{
    [XmlConfigFile(Category = "AnyUnreal")]
    public string? SdkRoot { get; set; }

    [XmlConfigFile(Category = "AnyUnreal")]
    public string? Dependencies { get; set; }

    [XmlConfigFile(Category = "AnyUnreal")]
    public string? CMake { get; set; }

    [XmlConfigFile(Category = "AnyUnreal")]
    public int WindowsShaderPlatformIndex { get; set; } = 45;

    [XmlConfigFile(Category = "AnyUnreal")]
    public int LinuxShaderPlatformIndex { get; set; } = 46;

    [XmlConfigFile(Category = "AnyUnreal")]
    public int Ps5ShaderPlatformIndex { get; set; } = 47;

    public bool IsConfigured => SdkRoot != null || Dependencies != null || CMake != null;

    public static AnyUnrealSettings Load()
    {
        AnyUnrealSettings settings = new();
        XmlConfig.ApplyTo(settings);
        return settings;
    }

    public UEBuildPlatform.StaticShader GetShaderPlatform(Target target)
    {
        foreach (int index in new[] { WindowsShaderPlatformIndex, LinuxShaderPlatformIndex, Ps5ShaderPlatformIndex })
        {
            if (index < 32 || index > 47) throw new InvalidOperationException("AnyUnreal shader indices must use UE's static platform range 32..47");
        }
        if (WindowsShaderPlatformIndex == LinuxShaderPlatformIndex || WindowsShaderPlatformIndex == Ps5ShaderPlatformIndex || LinuxShaderPlatformIndex == Ps5ShaderPlatformIndex) throw new InvalidOperationException("AnyUnreal shader platform indices must be distinct");
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
        if (string.IsNullOrWhiteSpace(SdkRoot) || string.IsNullOrWhiteSpace(Dependencies)) throw new InvalidOperationException("AnyUnreal requires SdkRoot and Dependencies in BuildConfiguration.xml");
        RequireCMake();
        return new SdkInstallation(SdkRoot, Dependencies);
    }

    public string RequireCMake()
    {
        if (string.IsNullOrWhiteSpace(CMake)) throw new InvalidOperationException("AnyUnreal requires the CMake executable path in BuildConfiguration.xml");
        return SdkInstallation.RequireFile(Path.GetFullPath(CMake));
    }
}
