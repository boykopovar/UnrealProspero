using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;
using UnrealProspero.Build.Execution;
using UnrealProspero.Build.Sdk;
using AutomationTool;
using EpicGames.Core;
using Microsoft.Extensions.Logging.Abstractions;
using UnrealBuildTool;

internal sealed class StagingChecks : BuildCommand
{
    public static void Run(string sdkRoot, string dependencies, string unrealRoot, string probes, string output)
    {
        output = Path.GetFullPath(output);
        if (Directory.Exists(output)) throw new IOException("Staging test output already exists");
        Directory.CreateDirectory(output);
        Environment.SetEnvironmentVariable(EnvVarNames.LocalRoot, Path.GetFullPath(unrealRoot));
        Environment.SetEnvironmentVariable(EnvVarNames.LogFolder, Path.Combine(output, "logs"));
        PropertyInfo buildMachine = typeof(Automation).GetProperty("IsBuildMachine", BindingFlags.Static | BindingFlags.Public) ?? throw new MissingMemberException("AutomationTool build environment is unavailable");
        buildMachine.SetValue(null, false);
        MethodInfo initialize = typeof(CommandUtils).GetMethod("InitCommandEnvironment", BindingFlags.Static | BindingFlags.NonPublic) ?? throw new MissingMethodException("AutomationTool environment initialization is unavailable");
        initialize.Invoke(null, null);
        PlatformExports.Initialize(Array.Empty<string>(), NullLogger.Instance);
        SdkInstallation sdk = new(sdkRoot, dependencies);
        foreach (UnrealTargetPlatform platformType in new[] { UnrealTargetPlatform.AnyWin64, UnrealTargetPlatform.AnyLinux, UnrealTargetPlatform.AnyPS5 })
        {
            Platform platform = Platform.GetPlatform(platformType);
            string projectDirectory = Path.Combine(output, platformType.ToString(), "StagingProbe");
            Directory.CreateDirectory(projectDirectory);
            FileReference project = new(Path.Combine(projectDirectory, "StagingProbe.uproject"));
            File.WriteAllText(project.FullName, "{\"FileVersion\":3}");
            {
                string metadata = Path.Combine(projectDirectory, "Build", "UnrealProspero");
                Directory.CreateDirectory(Path.GetDirectoryName(metadata)!);
                new ToolCommand(sdk.Executable, projectDirectory, new[] { "init", "StagingProbe", "--deps", sdk.Dependencies, "--directory", metadata }).ExecuteAsync(CancellationToken.None).GetAwaiter().GetResult();
            }
            string source = Path.Combine(probes, platformType.ToString(), "application");
            string binaries = Path.Combine(projectDirectory, "Binaries", platformType.ToString());
            foreach (string file in Directory.EnumerateFiles(source, "*", SearchOption.AllDirectories))
            {
                string destination = Path.Combine(binaries, Path.GetRelativePath(source, file));
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                File.Copy(file, destination);
            }
            FileReference executable = new(Path.Combine(binaries, "UnrealProbe" + Platform.GetExeExtension(platformType)));
            TargetReceipt receipt = new(project, "StagingProbe", TargetType.Game, platformType, UnrealTargetConfiguration.Development, new BuildVersion(), new UnrealArchitectures(UnrealArch.X64), false, BuildSettingsVersion.Latest, TargetBuildEnvironment.Unique);
            receipt.BuildProducts.Add(new BuildProduct(executable, BuildProductType.Executable));
            UnrealProsperoReceipt.Write(receipt.AdditionalProperties, sdk);
            List<UnrealTargetConfiguration> configurations = new() { UnrealTargetConfiguration.Development };
            DeploymentContext context = new(project, new DirectoryReference(unrealRoot), new DirectoryReference(Path.Combine(projectDirectory, "stage")), null, null, new DirectoryReference(Path.Combine(projectDirectory, "archive")), "", platform, platform, configurations, new[] { new StageTarget { Receipt = receipt, RequireFilesExist = true } }, new List<string> { "UnrealProbe" }, false, true, false, true, false, true, false, false, false, false, null, new List<DirectoryReference>());
            ProjectParams parameters = new(project, Command: new StagingChecks(), ClientConfigsToBuild: configurations, ClientTargetPlatforms: new List<TargetPlatformDescriptor> { new(platformType) }, Build: false, SkipCook: true, Stage: true);
            platform.GetFilesToDeployOrStage(parameters, context);
            foreach (string file in Directory.EnumerateFiles(binaries, "*", SearchOption.AllDirectories))
            {
                FileReference input = new(file);
                StagedFileReference staged = context.GetStagedFileLocation(input);
                if (!context.FilesToStage.NonUFSFiles.TryGetValue(staged, out FileReference? recorded) || recorded != input) throw new InvalidOperationException("Missing staged runtime file: " + file);
            }
            StagedFileReference metadataPath = new(platformType == UnrealTargetPlatform.AnyPS5 ? "sce_sys/param.json" : "app0/sce_sys/param.json");
            if (!context.FilesToStage.NonUFSFiles.ContainsKey(metadataPath)) throw new InvalidOperationException("Application metadata is missing from the runtime mount path");
            foreach (var file in context.FilesToStage.NonUFSFiles)
            {
                string destination = Path.Combine(context.StageDirectory.FullName, file.Key.Name);
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                File.Copy(file.Value.FullName, destination);
            }
            platform.Package(parameters, context, 0);
            platform.GetFilesToArchive(parameters, context);
            if (platformType == UnrealTargetPlatform.AnyPS5)
            {
                string image = context.StageDirectory.FullName + ".ffpfsc";
                if (new FileInfo(image).Length == 0 || context.ArchivedFiles.Count != 1 || new FileReference(context.ArchivedFiles.Keys.Single()) != new FileReference(image)) throw new InvalidOperationException("PS5 archive does not contain its image");
            }
            else if (context.ArchivedFiles.Count != context.FilesToStage.NonUFSFiles.Count) throw new InvalidOperationException("Archive does not contain all staged runtime files");
            if (platformType == UnrealTargetPlatform.AnyWin64 && OperatingSystem.IsWindows())
            {
                string stagedExecutable = Path.Combine(context.StageDirectory.FullName, context.GetStagedFileLocation(executable).Name);
                using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(30));
                new ToolCommand(stagedExecutable, context.StageDirectory.FullName, Array.Empty<string>()).ExecuteAsync(timeout.Token).GetAwaiter().GetResult();
                string videoProbe = SdkInstallation.RequireFile(Path.Combine(probes, platformType.ToString(), "video", "VideoOutputProbe.exe"));
                new ToolCommand(videoProbe, context.StageDirectory.FullName, Array.Empty<string>()).ExecuteAsync(timeout.Token).GetAwaiter().GetResult();
            }
            Console.WriteLine("Staged and packaged " + platformType);
        }
    }
}
