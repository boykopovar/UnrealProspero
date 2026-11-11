using System;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using UnrealProspero.Build.Execution;
using UnrealProspero.Build.Sdk;
using UnrealProspero.Build.Packaging;

internal static class Program
{
    private static async Task Main(string[] args)
    {
        if (args.Length != 2 && args.Length != 5) throw new ArgumentException("Expected SDK, dependencies and optionally project, ELF and output directories");
        SdkInstallation sdk = new(args[0], args[1]);
        foreach (Target target in Enum.GetValues<Target>())
        {
            sdk.ValidateTarget(target);
            Console.WriteLine("Validated " + SdkInstallation.GetTargetName(target));
        }
        reject<ArgumentException>(() => sdk.GetImportLibrary(Target.Ps5, "lib/../../libc"));
        reject<ArgumentOutOfRangeException>(() => SdkInstallation.GetTargetName((Target)99));
        reject<FileNotFoundException>(() => sdk.GetImportLibrary(Target.Ps5, "libUnrealProsperoMissing"));
        await new ToolCommand(sdk.Executable, Directory.GetCurrentDirectory(), new[] { "--version" }).ExecuteAsync(CancellationToken.None);
        try
        {
            await new ToolCommand(sdk.Executable, Directory.GetCurrentDirectory(), new[] { "invalid-unrealprospero-command" }).ExecuteAsync(CancellationToken.None);
            throw new Exception("Failed tool command was accepted");
        }
        catch (InvalidOperationException) { }
        using CancellationTokenSource cancellation = new();
        cancellation.Cancel();
        try
        {
            await new ToolCommand(sdk.Executable, Directory.GetCurrentDirectory(), new[] { "--version" }).ExecuteAsync(cancellation.Token);
            throw new Exception("Cancelled command was started");
        }
        catch (OperationCanceledException) { }
        Console.WriteLine("SDK validation and process checks passed");
        if (args.Length == 5)
        {
            string outputRoot = Path.GetFullPath(args[4]);
            if (Directory.Exists(outputRoot)) throw new IOException("Test output already exists");
            Directory.CreateDirectory(outputRoot);
            Ps5Package package = new(args[2], args[3], Path.Combine(args[2], "app0"), Path.Combine(outputRoot, "UnrealProspero.ffpfsc"), Path.Combine(outputRoot, "package workspace"));
            ToolCommand command = package.CreateCommand(sdk);
            if (!command.Arguments.Contains(package.Workspace)) throw new Exception("Workspace argument was not preserved");
            await package.BuildAsync(sdk, CancellationToken.None);
            reject<IOException>(() => new Ps5Package(args[2], args[3], Path.Combine(args[2], "app0"), package.Output, Path.Combine(outputRoot, "another workspace")));
            reject<InvalidDataException>(() => new Ps5Package(args[2], Path.Combine(args[2], "adk.json"), Path.Combine(args[2], "app0"), Path.Combine(outputRoot, "invalid.ffpfsc"), Path.Combine(outputRoot, "invalid workspace")));
            Console.WriteLine("PS5 packaging passed: " + new FileInfo(package.Output).Length + " bytes");
        }
    }

    private static void reject<TException>(Action action) where TException : Exception
    {
        try { action(); }
        catch (TException) { return; }
        throw new Exception("Expected " + typeof(TException).Name);
    }
}
