using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using AnyUnreal.Build.Execution;
using AnyUnreal.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging.Abstractions;
using UnrealBuildBase;
using UnrealBuildTool;

internal static class ToolchainChecks
{
    public static async Task RunAsync(SdkInstallation sdk, Target target, UnrealTargetPlatform platform, string cmake, string outputDirectory)
    {
        DirectoryReference intermediate = new(outputDirectory);
        Directory.CreateDirectory(outputDirectory);
        string sourceRoot = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Build");
        RuntimeProfile profile = await RuntimeProfile.ConfigureAsync(sdk, target, cmake, Path.Combine(sourceRoot, "Toolchain"), Path.Combine(outputDirectory, "profile"), CancellationToken.None);
        AnyUnrealToolChain toolchain = new(sdk, profile, target, cmake, Path.Combine(sourceRoot, "Linker"), ClangToolChainOptions.None, NullLogger.Instance);
        string extension = UEBuildPlatform.GetBuildPlatform(platform).GetBinaryExtension(UEBuildBinaryType.Executable);
        FileReference executable = FileReference.Combine(intermediate, "application", "UnrealProbe" + extension);
        TargetMakefile makefile = new("AnyUnreal", executable, FileReference.Combine(intermediate, "UnrealProbe.target"), intermediate, intermediate, TargetType.Program, new ConfigValueTracker(new Dictionary<ConfigDependencyKey, IReadOnlyList<string>?>()), false, null, null, null);
        TargetMakefileBuilder graph = new(makefile, new FileHashCache(false, false), NullLogger.Instance);
        SourceFileMetadataCache metadata = SourceFileMetadataCache.CreateHierarchy(null, NullLogger.Instance);
        UnrealArchitectures architectures = new(UnrealArch.X64);
        CppCompileEnvironment compile = new(platform, CppConfiguration.Development, architectures, metadata)
        {
            CppStandard = CppStandardVersion.Cpp20,
            bEnableExceptions = true,
            bUseRTTI = true,
            bOptimizeCode = true
        };
        CPPOutput objects = toolchain.CompileAllCPPFiles(compile, new[] { FileItem.GetItemByPath(Path.Combine(sourceRoot, "Toolchain/Probe.cpp")) }, DirectoryReference.Combine(intermediate, "objects"), "Probe", graph);
        CPPOutput entry = toolchain.CompileAllCPPFiles(compile, new[] { FileItem.GetItemByPath(Path.Combine(AppContext.BaseDirectory, "Entry.cpp")) }, DirectoryReference.Combine(intermediate, "entry"), "Entry", graph);
        LinkEnvironment archive = new(platform, CppConfiguration.Development, architectures)
        {
            IntermediateDirectory = intermediate,
            bIsBuildingLibrary = true
        };
        archive.InputFiles.AddRange(objects.ObjectFiles);
        archive.OutputFilePaths.Add(FileReference.Combine(intermediate, "Probe.a"));
        FileItem library = toolchain.LinkFiles(archive, false, graph);
        LinkEnvironment link = new(platform, CppConfiguration.Development, architectures) { IntermediateDirectory = intermediate };
        link.InputFiles.AddRange(entry.ObjectFiles);
        link.Libraries.Add(library.Location);
        link.OutputFilePaths.Add(executable);
        toolchain.LinkFiles(link, false, graph);
        graph.WaitOnWriteTasks();
        if (makefile.Actions.Count != 4) throw new InvalidOperationException("Unexpected UBT action count");
        IExternalAction executableLink = makefile.Actions.Last();
        string? startupInput = target switch
        {
            Target.Windows => Path.Combine(sdk.Root, "lib/libAnyDevKitWindowsStartup.a"),
            Target.Linux => Path.Combine(sdk.Root, "share/AnyDevKit/startup/LinuxStartup/src/Start.S"),
            _ => null
        };
        if (startupInput != null && !executableLink.PrerequisiteItems.Contains(FileItem.GetItemByPath(startupInput))) throw new InvalidOperationException("SDK startup changes would not invalidate the executable link");
        foreach (IExternalAction action in makefile.Actions)
        {
            foreach (FileItem input in action.PrerequisiteItems)
            {
                if (!File.Exists(input.AbsolutePath)) throw new FileNotFoundException("Missing action prerequisite", input.AbsolutePath);
            }
            Directory.CreateDirectory(action.WorkingDirectory.FullName);
            foreach (FileItem product in action.ProducedItems) Directory.CreateDirectory(Path.GetDirectoryName(product.AbsolutePath)!);
            await new ToolCommand(action.CommandPath.FullName, action.WorkingDirectory.FullName, CommandLineArguments.Split(action.CommandArguments)).ExecuteAsync(CancellationToken.None);
            if (action.ProducedItems.Any(item => !File.Exists(item.AbsolutePath))) throw new InvalidDataException("Action did not produce its declared outputs");
        }
        if (target == Target.Windows && OperatingSystem.IsWindows()) await new ToolCommand(executable.FullName, executable.Directory.FullName, Array.Empty<string>()).ExecuteAsync(CancellationToken.None);
        Console.WriteLine("Executed UBT compile, archive and link actions for " + platform);
    }
}
