using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using AnyUnreal.Build.Execution;
using AnyUnreal.Build.Sdk;

internal static class Program
{
    private static async Task Main(string[] args)
    {
        if (args.Length != 5 && args.Length != 7) throw new ArgumentException("Expected SDK, dependencies, CMake, profile source, output and optionally linker source and entry source");
        SdkInstallation sdk = new(args[0], args[1]);
        string source = Path.GetFullPath(args[3]);
        string output = Path.GetFullPath(args[4]);
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Output already exists");
        foreach (Target target in Enum.GetValues<Target>())
        {
            string directory = Path.Combine(output, SdkInstallation.GetTargetName(target));
            RuntimeProfile profile = await RuntimeProfile.ConfigureAsync(sdk, target, args[2], source, directory, CancellationToken.None);
            List<string> arguments = new(profile.GetCompileArguments());
            string objectFile = Path.Combine(directory, "Probe.o");
            arguments.AddRange(new[] { "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-c", Path.Combine(source, "Probe.cpp"), "-o", objectFile });
            await new ToolCommand(profile.Compiler, directory, arguments).ExecuteAsync(CancellationToken.None);
            if (!File.Exists(objectFile) || new FileInfo(objectFile).Length == 0) throw new InvalidDataException("Compiler did not produce an object");
            Console.WriteLine("Compiled runtime profile for " + target);
            if (args.Length == 7)
            {
                string entryObject = Path.Combine(directory, "Entry.o");
                List<string> entryArguments = new(profile.GetCompileArguments());
                entryArguments.AddRange(new[] { "-fPIC", "-c", Path.GetFullPath(args[6]), "-o", entryObject });
                await new ToolCommand(profile.Compiler, directory, entryArguments).ExecuteAsync(CancellationToken.None);
                string extension = target == Target.Windows ? ".exe" : target == Target.Ps5 ? ".elf" : "";
                string executable = Path.Combine(directory, "application", "AnyUnrealProbe" + extension);
                ToolCommand link = ExecutableLink.CreateCommand(sdk, target, args[2], args[5], Path.Combine(directory, "link"), executable, "Release", new[] { objectFile, entryObject }, Array.Empty<string>(), new[] { "libc", "libkernel" }, Array.Empty<string>(), Array.Empty<string>());
                await link.ExecuteAsync(CancellationToken.None);
                if (!File.Exists(executable) || new FileInfo(executable).Length == 0) throw new InvalidDataException("Missing executable");
                if (target == Target.Windows && OperatingSystem.IsWindows()) await new ToolCommand(executable, Path.GetDirectoryName(executable)!, Array.Empty<string>()).ExecuteAsync(CancellationToken.None);
                Console.WriteLine("Linked runtime profile for " + target);
            }
        }
    }
}
