using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using AnyUnreal.Build.Execution;
using AnyUnreal.Build.Sdk;

internal static class Program
{
    private static async Task Main(string[] args)
    {
        if (args.Length != 2) throw new ArgumentException("Expected SDK and dependency directories");
        SdkInstallation sdk = new(args[0], args[1]);
        foreach (Target target in Enum.GetValues<Target>())
        {
            sdk.ValidateTarget(target);
            Console.WriteLine("Validated " + sdk.GetTargetName(target));
        }
        reject<ArgumentException>(() => sdk.GetImportLibrary(Target.Ps5, "lib/../../libc"));
        reject<ArgumentOutOfRangeException>(() => sdk.GetTargetName((Target)99));
        reject<FileNotFoundException>(() => sdk.GetImportLibrary(Target.Ps5, "libAnyUnrealMissing"));
        await new ToolCommand(sdk.Executable, Directory.GetCurrentDirectory(), new[] { "--version" }).ExecuteAsync(CancellationToken.None);
        try
        {
            await new ToolCommand(sdk.Executable, Directory.GetCurrentDirectory(), new[] { "invalid-anyunreal-command" }).ExecuteAsync(CancellationToken.None);
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
    }

    private static void reject<TException>(Action action) where TException : Exception
    {
        try { action(); }
        catch (TException) { return; }
        throw new Exception("Expected " + typeof(TException).Name);
    }
}
