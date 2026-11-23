using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildBase;

namespace UnrealBuildTool;

internal sealed partial class UnrealProsperoToolChain : ClangToolChain
{
    private readonly SdkInstallation sdk;
    private readonly RuntimeProfile profile;
    private readonly Target target;
    private readonly string cmake;
    private readonly string linkerSource;

    public UnrealProsperoToolChain(SdkInstallation sdk, RuntimeProfile profile, Target target, string cmake, string linkerSource, ClangToolChainOptions options, ILogger logger) : base(options, logger)
    {
        if (profile.CompilerId != "Clang") throw new BuildException("Unreal requires the Clang runtime package");
        sdk.ValidateTarget(target);
        this.sdk = sdk;
        this.profile = profile;
        this.target = target;
        this.cmake = SdkInstallation.RequireFile(Path.GetFullPath(cmake));
        this.linkerSource = Path.GetFullPath(linkerSource);
        SdkInstallation.RequireFile(Path.Combine(this.linkerSource, "Run.cmake"));
    }

    protected override ClangToolChainInfo GetToolChainInfo() => new(null, new FileReference(profile.Compiler), new FileReference(profile.Archiver), Logger);

    protected override string GetPreprocessorDefinitionArgument(string definition) => quoteResponseArgument("-D" + definition);

    protected override void GetCompileArguments_Optimizations(CppCompileEnvironment environment, List<string> arguments)
    {
        if (environment.bCodeCoverage || environment.bPGOProfile || environment.bPGOOptimize) throw new BuildException("UnrealProspero does not provide a profiling runtime");
        base.GetCompileArguments_Optimizations(environment, arguments);
        arguments.Add(!environment.bOptimizeCode ? "-O0" : environment.OptimizationLevel switch
        {
            OptimizationMode.Speed => "-O3",
            OptimizationMode.Size => "-Oz",
            OptimizationMode.SizeAndSpeed => "-Os",
            _ => throw new BuildException("Unsupported optimization mode: {0}", environment.OptimizationLevel)
        });
        if (environment.bRetainFramePointers) arguments.Add("-fno-omit-frame-pointer");
    }

    protected override void GetCompileArguments_Global(CppCompileEnvironment environment, List<string> arguments)
    {
        if (environment.Architecture != UnrealArch.X64) throw new BuildException("UnrealProspero requires x86-64");
        base.GetCompileArguments_Global(environment, arguments);
        arguments.AddRange(profile.GetCompileArguments(false).Select(quoteResponseArgument));
        arguments.Add("-fno-builtin");
        arguments.Add("-fno-stack-protector");
        if (target != Target.Windows) arguments.Add("-fPIC");
    }

    protected override Action CompileCPPFile(CppCompileEnvironment environment, FileItem source, DirectoryReference outputDirectory, string moduleName, IActionGraphBuilder graph, IReadOnlyCollection<string> globalArguments, CPPOutput result)
    {
        if (moduleName is "Chaos" or "DataflowCore" or "BuildPatchServices" or "BandwidthMeasurementTool" or "DownlinkBandwidthManager") globalArguments = globalArguments.Append("-Wno-error=unused-result").ToArray();
        Action action = base.CompileCPPFile(environment, source, outputDirectory, moduleName, graph, globalArguments, result);
        action.bCanExecuteRemotely = false;
        action.bCanExecuteInUBA = false;
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(Path.Combine(sdk.Dependencies, "dependencies.json")));
        return action;
    }

    private static string quoteResponseArgument(string value) => "\"" + value.Replace("\\", "\\\\", StringComparison.Ordinal).Replace("\"", "\\\"", StringComparison.Ordinal) + "\"";
}
