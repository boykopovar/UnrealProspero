using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnrealProspero.Build.Execution;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using UnrealBuildBase;

namespace UnrealBuildTool;

internal sealed partial class UnrealProsperoToolChain
{
    public override void ModifyTargetReceipt(ReadOnlyTargetRules rules, TargetReceipt receipt) => UnrealProsperoReceipt.Write(receipt.AdditionalProperties, sdk);

    public override FileItem LinkFiles(LinkEnvironment environment, bool buildImportLibraryOnly, IActionGraphBuilder graph)
    {
        if (environment.Architecture != UnrealArch.X64) throw new BuildException("UnrealProspero requires x86-64");
        if (buildImportLibraryOnly || environment.bIsBuildingDLL) throw new BuildException("UnrealProspero applications require a monolithic executable");
        if (environment.IntermediateDirectory == null) throw new BuildException("Missing linker intermediate directory");
        if (environment.InputFiles.Count == 0) throw new BuildException("No linker object files supplied");
        if (environment.DelayLoadDLLs.Count != 0 || environment.ExcludedLibraries.Count != 0 || environment.SystemLibraryPaths.Count != 0) throw new BuildException("Host library search and delay loading are not supported by the SDK linker");
        if (environment.bCodeCoverage || environment.bPGOOptimize || environment.bPGOProfile) throw new BuildException("The SDK does not provide profiling or coverage runtimes");
        FileItem output = FileItem.GetItemByFileReference(environment.OutputFilePath);
        Action action = graph.CreateAction(ActionType.Link);
        action.WorkingDirectory = environment.IntermediateDirectory;
        action.RootPaths = environment.RootPaths;
        action.bCanExecuteRemotely = false;
        action.bCanExecuteInUBA = false;
        action.StatusDescription = environment.OutputFilePath.GetFileName();
        action.ProducedItems.Add(output);
        action.PrerequisiteItems.UnionWith(environment.InputFiles);
        action.PrerequisiteItems.UnionWith(environment.Libraries.Select(FileItem.GetItemByFileReference));
        if (environment.bIsBuildingLibrary)
        {
            if (environment.SystemLibraries.Count != 0 || environment.Libraries.Count != 0 || !string.IsNullOrWhiteSpace(environment.AdditionalArguments)) throw new BuildException("Archive actions accept object files only");
            FileReference responsePath = FileReference.Combine(environment.IntermediateDirectory, environment.OutputFilePath.GetFileName() + ".rsp");
            FileItem response = graph.CreateIntermediateTextFile(responsePath, environment.InputFiles.Select(item => quoteResponseArgument(item.AbsolutePath)));
            action.PrerequisiteItems.Add(response);
            action.DeleteItems.Add(output);
            action.CommandPath = new FileReference(profile.Archiver);
            action.CommandArguments = "rcs " + CommandLineArguments.Quote(output.AbsolutePath) + " " + CommandLineArguments.Quote("@" + response.AbsolutePath);
            action.CommandDescription = "Archive";
            return output;
        }
        List<string> options = new(CommandLineArguments.Split(environment.AdditionalArguments));
        if (environment.bAllowLTCG) options.Add(Options.HasFlag(ClangToolChainOptions.EnableThinLTO) ? "-flto=thin" : "-flto");
        if (environment.bCreateMapFile || target == Target.Ps5)
        {
            string mapFile = output.AbsolutePath + ".map";
            options.Add("-Wl,-Map," + mapFile);
            if (target == Target.Ps5) options.Add("-Wl,--cref");
            action.ProducedItems.Add(FileItem.GetItemByPath(mapFile));
        }
        string configuration = environment.Configuration switch { CppConfiguration.Debug => "Debug", CppConfiguration.Development => "RelWithDebInfo", CppConfiguration.Shipping => "Release", _ => throw new BuildException("Unknown linker configuration") };
        string buildDirectory = Path.Combine(environment.IntermediateDirectory.FullName, "UnrealProsperoLink", environment.OutputFilePath.GetFileName());
        string[] imports = environment.SystemLibraries.Concat(new[] { "libc", "libkernel" }).Distinct(StringComparer.Ordinal).ToArray();
        ToolCommand command = ExecutableLink.CreateCommand(sdk, target, cmake, linkerSource, buildDirectory, output.AbsolutePath, configuration, environment.InputFiles.Select(item => item.AbsolutePath), environment.Libraries.Select(item => item.FullName), imports, Array.Empty<string>(), options);
        action.CommandPath = new FileReference(command.Executable);
        action.CommandArguments = CommandLineArguments.Join(command.Arguments);
        action.WorkingDirectory = new DirectoryReference(command.WorkingDirectory);
        action.CommandDescription = "Link through AnyDevKit";
        FileItem manifest = FileItem.GetItemByPath(Path.Combine(buildDirectory, "LinkInputs.cmake"));
        graph.CreateIntermediateTextFile(manifest, File.ReadAllText(manifest.AbsolutePath));
        action.PrerequisiteItems.Add(manifest);
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(sdk.Executable));
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(profile.RuntimeLibrary));
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(sdk.GetCxxConfiguration(target)));
        if (target == Target.Windows) action.PrerequisiteItems.Add(FileItem.GetItemByPath(SdkInstallation.RequireFile(Path.Combine(sdk.Root, "lib/libAnyDevKitWindowsStartup.a"))));
        if (target == Target.Linux)
        {
            string startup = SdkInstallation.RequireDirectory(Path.Combine(sdk.Root, "share/AnyDevKit/startup"));
            foreach (string file in Directory.EnumerateFiles(startup, "*", SearchOption.AllDirectories)) action.PrerequisiteItems.Add(FileItem.GetItemByPath(file));
        }
        foreach (string import in imports) action.PrerequisiteItems.Add(FileItem.GetItemByPath(sdk.GetImportLibrary(target, import)));
        foreach (string file in Directory.EnumerateFiles(linkerSource, "*.cmake")) action.PrerequisiteItems.Add(FileItem.GetItemByPath(file));
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(Path.Combine(linkerSource, "CMakeLists.txt")));
        foreach (string file in Directory.EnumerateFiles(Path.Combine(sdk.Root, "lib/cmake/AnyDevKit"), "*.cmake", SearchOption.AllDirectories)) action.PrerequisiteItems.Add(FileItem.GetItemByPath(file));
        return output;
    }
}
