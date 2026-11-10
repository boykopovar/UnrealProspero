using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using AnyUnreal.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildBase;

namespace UnrealBuildTool;

internal sealed partial class AnyUnrealToolChain : ClangToolChain
{
    private readonly SdkInstallation sdk;
    private readonly RuntimeProfile profile;
    private readonly Target target;
    private readonly string cmake;
    private readonly string linkerSource;

    public AnyUnrealToolChain(SdkInstallation sdk, RuntimeProfile profile, Target target, string cmake, string linkerSource, ClangToolChainOptions options, ILogger logger) : base(options, logger)
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

    protected override void GetCompileArguments_Global(CppCompileEnvironment environment, List<string> arguments)
    {
        if (environment.Architecture != UnrealArch.X64) throw new BuildException("AnyUnreal requires x86-64");
        base.GetCompileArguments_Global(environment, arguments);
        arguments.AddRange(profile.GetCompileArguments(false).Select(quoteResponseArgument));
        arguments.Add("-fno-builtin");
        arguments.Add("-fno-stack-protector");
        if (target != Target.Windows) arguments.Add("-fPIC");
    }

    protected override Action CompileCPPFile(CppCompileEnvironment environment, FileItem source, DirectoryReference outputDirectory, string moduleName, IActionGraphBuilder graph, IReadOnlyCollection<string> globalArguments, CPPOutput result)
    {
        Action action = base.CompileCPPFile(environment, source, outputDirectory, moduleName, graph, globalArguments, result);
        action.bCanExecuteRemotely = false;
        action.bCanExecuteInUBA = false;
        action.PrerequisiteItems.Add(FileItem.GetItemByPath(Path.Combine(sdk.Dependencies, "dependencies.json")));
        return action;
    }

    private static string quoteResponseArgument(string value) => "\"" + value.Replace("\\", "\\\\", StringComparison.Ordinal).Replace("\"", "\\\"", StringComparison.Ordinal) + "\"";
}
