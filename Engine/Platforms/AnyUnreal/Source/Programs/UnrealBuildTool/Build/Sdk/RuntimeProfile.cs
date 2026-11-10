using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using AnyUnreal.Build.Execution;

namespace AnyUnreal.Build.Sdk;

public sealed class RuntimeProfile
{
    public string Compiler { get; }
    public string CompilerId { get; }
    public string CompilerTarget { get; }
    public string Sysroot { get; }
    public string RuntimeLibrary { get; }
    public IReadOnlyList<string> CompileOptions { get; }
    public IReadOnlyList<string> Definitions { get; }
    public IReadOnlyList<string> IncludeDirectories { get; }
    public IReadOnlyList<string> LinkOptions { get; }

    private RuntimeProfile(string directory)
    {
        Compiler = SdkInstallation.RequireFile(readValue(directory, "Compiler"));
        CompilerId = readValue(directory, "CompilerId");
        if (CompilerId != "Clang" && CompilerId != "GNU") throw new InvalidDataException("Unsupported SDK compiler: " + CompilerId);
        CompilerTarget = readValue(directory, "CompilerTarget", true);
        Sysroot = SdkInstallation.RequireDirectory(readValue(directory, "Sysroot"));
        RuntimeLibrary = SdkInstallation.RequireFile(readValue(directory, "RuntimeLibrary"));
        CompileOptions = Array.AsReadOnly(readList(directory, "CompilerFlags").Concat(readList(directory, "CompileOptions")).ToArray());
        Definitions = Array.AsReadOnly(readList(directory, "CompileDefinitions"));
        IncludeDirectories = Array.AsReadOnly(readList(directory, "IncludeDirectories").Select(SdkInstallation.RequireDirectory).ToArray());
        LinkOptions = Array.AsReadOnly(readList(directory, "LinkOptions"));
        if (IncludeDirectories.Count == 0) throw new InvalidDataException("Runtime profile has no include directories");
    }

    public static async Task<RuntimeProfile> ConfigureAsync(SdkInstallation sdk, Target target, string cmake, string sourceDirectory, string buildDirectory, CancellationToken cancellationToken)
    {
        sdk.ValidateTarget(target);
        sourceDirectory = Path.GetFullPath(sourceDirectory);
        buildDirectory = Path.GetFullPath(buildDirectory);
        SdkInstallation.RequireFile(Path.Combine(sourceDirectory, "CMakeLists.txt"));
        if (Directory.Exists(buildDirectory) || File.Exists(buildDirectory)) throw new IOException("Profile build directory already exists: " + buildDirectory);
        List<string> arguments = new()
        {
            "-S", sourceDirectory, "-B", buildDirectory, "-G", "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_TOOLCHAIN_FILE=" + sdk.GetToolchain(target).Replace('\\', '/'),
            "-DAnyDevKit_DIR=" + Path.Combine(sdk.Root, "lib/cmake/AnyDevKit").Replace('\\', '/'),
            "-DADK_DEPENDENCY_ROOT=" + sdk.GetTargetRoot(target).Replace('\\', '/'),
            "-DADK_SYSROOT=" + sdk.GetSysroot(target).Replace('\\', '/'),
            "-DADK_COMPILER=" + sdk.Compiler
        };
        await new ToolCommand(cmake, sourceDirectory, arguments).ExecuteAsync(cancellationToken);
        RuntimeProfile result = new(Path.Combine(buildDirectory, "profile"));
        string expectedCompiler = sdk.Compiler == "clang" ? "Clang" : "GNU";
        if (result.CompilerId != expectedCompiler) throw new InvalidDataException("Compiler does not match the installed runtime");
        return result;
    }

    public IReadOnlyList<string> GetCompileArguments()
    {
        List<string> arguments = new();
        if (CompilerTarget.Length != 0) arguments.Add("--target=" + CompilerTarget);
        arguments.Add("--sysroot=" + Sysroot);
        arguments.AddRange(CompileOptions);
        arguments.Add("-std=c++20");
        foreach (string definition in Definitions) arguments.Add("-D" + definition);
        foreach (string directory in IncludeDirectories)
        {
            arguments.Add("-isystem");
            arguments.Add(directory);
        }
        return arguments.AsReadOnly();
    }

    private static string readValue(string directory, string name, bool allowEmpty = false)
    {
        string[] lines = File.ReadAllLines(SdkInstallation.RequireFile(Path.Combine(directory, name + ".txt")));
        if (lines.Length != 1 || (!allowEmpty && string.IsNullOrWhiteSpace(lines[0]))) throw new InvalidDataException("Invalid profile field: " + name);
        return lines[0];
    }

    private static string[] readList(string directory, string name)
    {
        string[] lines = File.ReadAllLines(SdkInstallation.RequireFile(Path.Combine(directory, name + ".txt")));
        if (lines.Length == 1 && lines[0].Length == 0) return Array.Empty<string>();
        if (lines.Any(string.IsNullOrWhiteSpace)) throw new InvalidDataException("Empty argument in runtime profile: " + name);
        return lines;
    }
}
