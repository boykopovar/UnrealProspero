using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace UnrealProspero.Build.Sdk;

public sealed class SdkInstallation
{
    public string Root { get; }
    public string Dependencies { get; }
    public string Executable { get; }
    public string Headers { get; }
    public string AnyPs5Version { get; }
    public string CxxVersion { get; }
    public string Compiler { get; }
    private readonly HashSet<Target> targets = new();

    public SdkInstallation(string root, string dependencies)
    {
        Root = Path.GetFullPath(root);
        Dependencies = Path.GetFullPath(dependencies);
        Executable = RequireFile(Path.Combine(Root, "bin", OperatingSystem.IsWindows() ? "adk.exe" : "adk"));
        Headers = Path.Combine(Dependencies, "include", "anyps5");
        RequireFile(Path.Combine(Root, "lib", "cmake", "AnyDevKit", "AnyDevKitConfig.cmake"));
        RequireFile(Path.Combine(Headers, "prx", "libc", "libc.hpp"));
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(RequireFile(Path.Combine(Dependencies, "dependencies.json"))));
        JsonElement metadata = document.RootElement;
        if (metadata.GetProperty("schemaVersion").GetInt32() != 1) throw new InvalidDataException("Unsupported dependency schema");
        AnyPs5Version = readString(metadata, "anyps5");
        CxxVersion = readString(metadata, "anycxx");
        Compiler = readString(metadata, "compiler");
        if (Compiler != "clang" && Compiler != "gcc") throw new InvalidDataException("Unsupported dependency compiler");
        foreach (JsonElement item in metadata.GetProperty("targets").EnumerateArray())
        {
            Target target = item.GetString() switch
            {
                "windows" => Target.Windows,
                "linux" => Target.Linux,
                "ps5" => Target.Ps5,
                _ => throw new InvalidDataException("Unknown dependency target")
            };
            if (!targets.Add(target)) throw new InvalidDataException("Duplicate dependency target");
        }
        if (targets.Count == 0) throw new InvalidDataException("No dependency targets installed");
    }

    public static string GetTargetName(Target target) => target switch
    {
        Target.Windows => "windows",
        Target.Linux => "linux",
        Target.Ps5 => "ps5",
        _ => throw new ArgumentOutOfRangeException(nameof(target))
    };

    public string GetTargetRoot(Target target)
    {
        if (!targets.Contains(target)) throw new InvalidOperationException("Target dependencies are not installed: " + target);
        return RequireDirectory(Path.Combine(Dependencies, GetTargetName(target)));
    }

    public string GetToolchain(Target target)
    {
        GetTargetRoot(target);
        return RequireFile(Path.Combine(Root, "lib", "cmake", "AnyDevKit", "toolchains", GetTargetName(target) + ".cmake"));
    }

    public string GetSysroot(Target target) => RequireDirectory(Path.Combine(GetTargetRoot(target), "sysroot"));

    public string GetCxxConfiguration(Target target) => RequireFile(Path.Combine(GetTargetRoot(target), "anycxx", "lib", "cmake", "AnyLibcxx", "AnyLibcxxConfig.cmake"));

    public string GetImportLibrary(Target target, string library)
    {
        ValidateLibraryName(library);
        return RequireFile(Path.Combine(GetTargetRoot(target), "imports", library + (target == Target.Windows ? ".a" : ".prx")));
    }

    public static void ValidateLibraryName(string library)
    {
        if (!library.StartsWith("lib", StringComparison.Ordinal) || library.Length <= 3) throw new ArgumentException("Invalid import library name", nameof(library));
        foreach (char value in library)
        {
            if (!char.IsAsciiLetterOrDigit(value) && value != '_' && value != '.') throw new ArgumentException("Invalid import library name", nameof(library));
        }
    }

    public void ValidateTarget(Target target)
    {
        GetToolchain(target);
        GetCxxConfiguration(target);
        string sysroot = GetSysroot(target);
        RequireDirectory(Path.Combine(sysroot, target == Target.Windows ? "include" : "usr/include"));
        GetImportLibrary(target, "libc");
        GetImportLibrary(target, "libkernel");
        if (target != Target.Ps5) RequireFile(Path.Combine(GetTargetRoot(target), "runtime", "libc.prx"));
    }

    public static string RequireFile(string path)
    {
        if (!File.Exists(path)) throw new FileNotFoundException("Required SDK file is missing", path);
        return path;
    }

    public static string RequireDirectory(string path)
    {
        if (!Directory.Exists(path)) throw new DirectoryNotFoundException("Required SDK directory is missing: " + path);
        return path;
    }

    private static string readString(JsonElement metadata, string name)
    {
        string? value = metadata.GetProperty(name).GetString();
        if (string.IsNullOrWhiteSpace(value)) throw new InvalidDataException("Empty dependency field: " + name);
        return value;
    }
}
