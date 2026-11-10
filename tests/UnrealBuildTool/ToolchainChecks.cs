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
        CppCompileEnvironment platformCompile = new(compile);
        platformCompile.UserIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Platforms/AnyUnreal/Source/Runtime/Core/Public"));
        platformCompile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime/Core/Public"));
        platformCompile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime/TraceLog/Public"));
        platformCompile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime/AutoRTFM/Public"));
        platformCompile.Definitions.AddRange(new[] { "OVERRIDE_PLATFORM_HEADER_NAME=AnyUnreal", "ANYUNREAL_TARGET_" + SdkInstallation.GetTargetName(target).ToUpperInvariant() + "=1", "UE_BUILD_DEVELOPMENT=1", "WITH_EDITOR=0", "WITH_ENGINE=0", "WITH_UNREAL_DEVELOPER_TOOLS=0", "WITH_PLUGIN_SUPPORT=0", "IS_MONOLITHIC=1", "IS_PROGRAM=1", "WITH_SERVER_CODE=1", "UE_MERGED_MODULES=0" });
        platformCompile.Definitions.Add("CORE_API=");
        platformCompile.Definitions.Add("TRACELOG_API=");
        toolchain.CompileAllCPPFiles(platformCompile, new[] { FileItem.GetItemByPath(Path.Combine(AppContext.BaseDirectory, "PlatformTypes.cpp")) }, DirectoryReference.Combine(intermediate, "platform"), "PlatformTypes", graph);
        CppCompileEnvironment threadingCompile = new(platformCompile);
        threadingCompile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime/Core/Private"));
        toolchain.CompileAllCPPFiles(threadingCompile, new[] { FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Source/Runtime/Core/Private/HAL/ThreadingBase.cpp")) }, DirectoryReference.Combine(intermediate, "unreal"), "ThreadingBase", graph);
        CppCompileEnvironment allocatorCompile = new(threadingCompile);
        allocatorCompile.Definitions.AddRange(new[] { "__UNREAL__=1", "AUTORTFM_API=" });
        allocatorCompile.ForceIncludeFiles.Add(FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Source/Runtime/Core/Public/CoreMinimal.h")));
        toolchain.CompileAllCPPFiles(allocatorCompile, new[] { FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Source/Runtime/Core/Private/HAL/MallocBinned3.cpp")) }, DirectoryReference.Combine(intermediate, "unreal"), "MallocBinned3", graph);
        CppCompileEnvironment llmCompile = new(threadingCompile);
        llmCompile.Definitions.Add("LLM_ENABLED_IN_CONFIG=1");
        toolchain.CompileAllCPPFiles(llmCompile, new[] { FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Source/Runtime/Core/Private/HAL/LLM/LLM.cpp")) }, DirectoryReference.Combine(intermediate, "unreal"), "LLM", graph);
        CppCompileEnvironment launchCompile = new(platformCompile);
        launchCompile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime/Launch/Public"));
        toolchain.CompileAllCPPFiles(launchCompile, new[] { FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Source/Runtime/Launch/Private/AnyUnreal/LaunchAnyUnreal.cpp")) }, DirectoryReference.Combine(intermediate, "launch"), "Launch", graph);
        CPPOutput entry = toolchain.CompileAllCPPFiles(platformCompile, new[] { "PlatformRuntime.cpp", "MutexChecks.cpp", "EventChecks.cpp", "AtomicChecks.cpp", "MathChecks.cpp", "MiscChecks.cpp", "GuidChecks.cpp", "ExitChecks.cpp", "ArgumentChecks.cpp", "StringChecks.cpp", "ClockChecks.cpp", "MemoryChecks.cpp", "VirtualMemoryChecks.cpp", "TimeChecks.cpp", "ProcessChecks.cpp", "ModuleChecks.cpp", "ThreadChecks.cpp", "FileChecks.cpp", "FileLockChecks.cpp", "FileSystemChecks.cpp", "FileMetadataChecks.cpp", "DirectoryChecks.cpp" }.Select(name => FileItem.GetItemByPath(Path.Combine(AppContext.BaseDirectory, name))).ToArray(), DirectoryReference.Combine(intermediate, "entry"), "Entry", graph);
        string coreSource = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Source/Runtime/Core/Private/AnyUnreal");
        CPPOutput platformObjects = toolchain.CompileAllCPPFiles(platformCompile, new[] { "AnyUnrealPlatformTLS.cpp", "Mutex/RecursiveMutex.cpp", "Mutex/SharedMutex.cpp", "Kernel/Event.cpp", "AnyUnrealEvent.cpp", "AnyUnrealPlatformMisc.cpp", "AnyUnrealPlatformGuid.cpp", "Text/Encoding.cpp", "Kernel/Debug.cpp", "Kernel/Clock.cpp", "Kernel/SystemConfiguration.cpp", "Kernel/DirectAllocation.cpp", "Memory/PageAllocator.cpp", "Memory/VirtualMemoryBlock.cpp", "Kernel/Calendar.cpp", "AnyUnrealPlatformTime.cpp", "AnyUnrealPlatformProcess.cpp", "Process/Events.cpp", "Process/Sleep.cpp", "Process/Identity.cpp", "Process/Threads.cpp", "Process/Modules.cpp", "Process/Exit.cpp", "Process/RequestExit.cpp", "Process/Arguments.cpp", "AnyUnrealRunnableThread.cpp", "AnyUnrealPlatformStackWalk.cpp", "Runnable/Priority.cpp", "Kernel/Module.cpp", "Kernel/Affinity.cpp", "Kernel/Thread.cpp", "Kernel/ThreadAttributes.cpp", "Kernel/File.cpp", "Kernel/FileLock.cpp", "Kernel/Descriptor.cpp", "Kernel/Directory.cpp", "File/FileHandle.cpp", "Kernel/FileSystem.cpp", "Kernel/Move.cpp", "File/Timestamp.cpp", "File/Metadata.cpp", "File/DirectoryVisitor.cpp", "File/Path.cpp", "AnyUnrealPlatformFile.cpp", "File/PlatformMetadata.cpp", "File/PlatformDirectories.cpp", "File/PlatformHandles.cpp" }.Select(name => FileItem.GetItemByPath(Path.Combine(coreSource, name))).ToArray(), DirectoryReference.Combine(intermediate, "core"), "Core", graph);
        LinkEnvironment archive = new(platform, CppConfiguration.Development, architectures)
        {
            IntermediateDirectory = intermediate,
            bIsBuildingLibrary = true
        };
        archive.InputFiles.AddRange(objects.ObjectFiles);
        archive.InputFiles.AddRange(platformObjects.ObjectFiles);
        archive.OutputFilePaths.Add(FileReference.Combine(intermediate, "Probe.a"));
        FileItem library = toolchain.LinkFiles(archive, false, graph);
        LinkEnvironment link = new(platform, CppConfiguration.Development, architectures) { IntermediateDirectory = intermediate };
        link.InputFiles.AddRange(entry.ObjectFiles);
        link.Libraries.Add(library.Location);
        link.SystemLibraries.Add("libkernel_sys");
        link.OutputFilePaths.Add(executable);
        toolchain.LinkFiles(link, false, graph);
        CppCompileEnvironment videoCompile = new(platformCompile);
        videoCompile.UserIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Platforms/AnyUnreal/Source/Runtime/ApplicationCore/Public"));
        CPPOutput video = toolchain.CompileAllCPPFiles(videoCompile, new[]
        {
            FileItem.GetItemByPath(Path.Combine(AppContext.BaseDirectory, "VideoOutputChecks.cpp")),
            FileItem.GetItemByPath(Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Source/Runtime/ApplicationCore/Private/AnyUnreal/Video/Output.cpp"))
        }, DirectoryReference.Combine(intermediate, "video-objects"), "VideoOutput", graph);
        FileReference videoExecutable = FileReference.Combine(intermediate, "video", "VideoOutputProbe" + extension);
        LinkEnvironment videoLink = new(platform, CppConfiguration.Development, architectures) { IntermediateDirectory = intermediate };
        videoLink.InputFiles.AddRange(video.ObjectFiles);
        videoLink.Libraries.Add(library.Location);
        videoLink.SystemLibraries.Add("libSceVideoOut");
        videoLink.OutputFilePaths.Add(videoExecutable);
        toolchain.LinkFiles(videoLink, false, graph);
        graph.WaitOnWriteTasks();
        if (makefile.Actions.Count != 80) throw new InvalidOperationException("Unexpected UBT action count");
        string? startupInput = target switch
        {
            Target.Windows => Path.Combine(sdk.Root, "lib/libAnyDevKitWindowsStartup.a"),
            Target.Linux => Path.Combine(sdk.Root, "share/AnyDevKit/startup/LinuxStartup/src/Start.S"),
            _ => null
        };
        if (startupInput != null)
        {
            foreach (FileReference output in new[] { executable, videoExecutable })
            {
                IExternalAction executableLink = makefile.Actions.Single(action => action.ProducedItems.Contains(FileItem.GetItemByPath(output.FullName)));
                if (!executableLink.PrerequisiteItems.Contains(FileItem.GetItemByPath(startupInput))) throw new InvalidOperationException("SDK startup changes would not invalidate the executable link");
            }
        }
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
        await VideoOutputChecks.PrepareAsync(sdk, outputDirectory, videoExecutable.Directory.FullName);
        if (target == Target.Windows && OperatingSystem.IsWindows())
        {
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(30));
            await new ToolCommand(executable.FullName, executable.Directory.FullName, Array.Empty<string>()).ExecuteAsync(timeout.Token);
            await FileLockChecks.RunAsync(executable.FullName);
            await ExitChecks.RunAsync(executable.FullName);
            await ArgumentChecks.RunAsync(executable.FullName);
            using CancellationTokenSource videoTimeout = new(TimeSpan.FromSeconds(30));
            await new ToolCommand(videoExecutable.FullName, videoExecutable.Directory.FullName, Array.Empty<string>()).ExecuteAsync(videoTimeout.Token);
        }
        Console.WriteLine("Executed UBT compile, archive and link actions for " + platform);
    }
}
