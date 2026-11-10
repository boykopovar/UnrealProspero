using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace AnyUnreal.Build.Execution;

public sealed class ToolCommand
{
    public string Executable { get; }
    public string WorkingDirectory { get; }
    public IReadOnlyList<string> Arguments { get; }

    public ToolCommand(string executable, string workingDirectory, IEnumerable<string> arguments)
    {
        Executable = Path.GetFullPath(executable);
        WorkingDirectory = Path.GetFullPath(workingDirectory);
        if (!File.Exists(Executable)) throw new FileNotFoundException("Tool executable is missing", Executable);
        if (!Directory.Exists(WorkingDirectory)) throw new DirectoryNotFoundException(WorkingDirectory);
        List<string> values = new();
        foreach (string argument in arguments)
        {
            if (argument is null || argument.Contains('\0', StringComparison.Ordinal)) throw new ArgumentException("Invalid tool argument", nameof(arguments));
            values.Add(argument);
        }
        Arguments = new ReadOnlyCollection<string>(values);
    }

    public async Task ExecuteAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        ProcessStartInfo start = new(Executable)
        {
            WorkingDirectory = WorkingDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        foreach (string argument in Arguments) start.ArgumentList.Add(argument);
        using Process process = Process.Start(start) ?? throw new InvalidOperationException("Cannot start " + Executable);
        Task output = process.StandardOutput.BaseStream.CopyToAsync(Console.OpenStandardOutput(), CancellationToken.None);
        Task error = process.StandardError.BaseStream.CopyToAsync(Console.OpenStandardError(), CancellationToken.None);
        try
        {
            await process.WaitForExitAsync(cancellationToken).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            if (!process.HasExited) process.Kill(true);
            await process.WaitForExitAsync(CancellationToken.None).ConfigureAwait(false);
            await Task.WhenAll(output, error).ConfigureAwait(false);
            throw;
        }
        await Task.WhenAll(output, error).ConfigureAwait(false);
        if (process.ExitCode != 0) throw new InvalidOperationException(Executable + " exited with code " + process.ExitCode);
    }
}
