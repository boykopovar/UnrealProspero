using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using AnyUnreal.Build.Execution;

namespace AnyUnreal.Build.Sdk;

public sealed class ExecutableLink
{
    public static ToolCommand CreateCommand(SdkInstallation sdk, Target target, string cmake, string sourceDirectory, string buildDirectory, string output, string configuration, IEnumerable<string> objects, IEnumerable<string> libraries, IEnumerable<string> imports, IEnumerable<string> runtimeLibraries, IEnumerable<string> linkOptions)
    {
        sdk.ValidateTarget(target);
        sourceDirectory = Path.GetFullPath(sourceDirectory);
        buildDirectory = Path.GetFullPath(buildDirectory);
        output = Path.GetFullPath(output);
        string script = SdkInstallation.RequireFile(Path.Combine(sourceDirectory, "Run.cmake"));
        SdkInstallation.RequireFile(Path.Combine(sourceDirectory, "CMakeLists.txt"));
        string extension = target switch { Target.Windows => ".exe", Target.Ps5 => ".elf", Target.Linux => "", _ => throw new ArgumentOutOfRangeException(nameof(target)) };
        if (Path.GetExtension(output) != extension) throw new ArgumentException("Unexpected executable extension", nameof(output));
        if (configuration != "Debug" && configuration != "Release" && configuration != "RelWithDebInfo") throw new ArgumentException("Unknown link configuration", nameof(configuration));
        string[] inputObjects = objects.Select(Path.GetFullPath).ToArray();
        string[] importNames = imports.Distinct(StringComparer.Ordinal).ToArray();
        string[] runtimeNames = runtimeLibraries.Distinct(StringComparer.Ordinal).ToArray();
        if (inputObjects.Length == 0) throw new ArgumentException("No object files supplied", nameof(objects));
        foreach (string name in importNames) sdk.GetImportLibrary(target, name);
        foreach (string name in runtimeNames) SdkInstallation.ValidateLibraryName(name);
        StringBuilder manifest = new();
        write(manifest, "NAME", new[] { Path.GetFileNameWithoutExtension(output) });
        write(manifest, "OUTPUT", new[] { output });
        write(manifest, "OUTPUT_DIRECTORY", new[] { Path.GetDirectoryName(output)! });
        write(manifest, "BUILD_DIRECTORY", new[] { buildDirectory });
        write(manifest, "CONFIGURATION", new[] { configuration });
        write(manifest, "TOOLCHAIN", new[] { sdk.GetToolchain(target) });
        write(manifest, "SDK_CONFIG", new[] { Path.Combine(sdk.Root, "lib/cmake/AnyDevKit") });
        write(manifest, "DEPENDENCIES", new[] { sdk.GetTargetRoot(target) });
        write(manifest, "SYSROOT", new[] { sdk.GetSysroot(target) });
        write(manifest, "COMPILER", new[] { sdk.Compiler });
        write(manifest, "OBJECTS", inputObjects);
        write(manifest, "LIBRARIES", libraries.Select(Path.GetFullPath));
        write(manifest, "IMPORTS", importNames);
        write(manifest, "RUNTIME_LIBRARIES", runtimeNames);
        write(manifest, "LINK_OPTIONS", linkOptions, false);
        Directory.CreateDirectory(buildDirectory);
        string manifestFile = Path.Combine(buildDirectory, "LinkInputs.cmake");
        string contents = manifest.ToString();
        if (!File.Exists(manifestFile) || File.ReadAllText(manifestFile) != contents) File.WriteAllText(manifestFile, contents, new UTF8Encoding(false));
        return new ToolCommand(cmake, buildDirectory, new[] { "-DANYUNREAL_MANIFEST=" + manifestFile.Replace('\\', '/'), "-P", script });
    }

    private static void write(StringBuilder manifest, string name, IEnumerable<string> values, bool normalizePaths = true)
    {
        manifest.Append("set(ANYUNREAL_").Append(name);
        foreach (string value in values)
        {
            if (string.IsNullOrEmpty(value) || value.IndexOfAny(new[] { '\0', '\r', '\n', ';' }) >= 0) throw new ArgumentException("Unsupported CMake list value", nameof(values));
            string normalized = normalizePaths ? value.Replace('\\', '/') : value;
            string delimiter = "=";
            while (normalized.Contains("]" + delimiter + "]", StringComparison.Ordinal)) delimiter += "=";
            manifest.Append(" [").Append(delimiter).Append('[').Append(normalized).Append(']').Append(delimiter).Append(']');
        }
        manifest.AppendLine(")");
    }
}
