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

    public bool IsConfigured => SdkRoot != null || Dependencies != null || CMake != null;

    public static AnyUnrealSettings Load()
    {
        AnyUnrealSettings settings = new();
        XmlConfig.ApplyTo(settings);
        return settings;
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
