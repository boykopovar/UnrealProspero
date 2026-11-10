using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

internal static class ArgumentChecks
{
    public static async Task RunAsync(string executable)
    {
        ProcessStartInfo info = new(executable)
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            WorkingDirectory = Path.GetDirectoryName(executable)!
        };
        foreach (string argument in new[] { "--arguments-probe", "-name=two words", "\"quoted\"", "path\\", "\u0411\u6587\U0001f642", "" }) info.ArgumentList.Add(argument);
        using Process process = Process.Start(info) ?? throw new InvalidOperationException("Could not start argument probe");
        try
        {
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(10));
            await process.WaitForExitAsync(timeout.Token);
            if (process.ExitCode != 0) throw new InvalidOperationException($"Argument probe failed: {process.ExitCode}");
        }
        finally
        {
            if (!process.HasExited) process.Kill(true);
            if (!process.WaitForExit(10000)) throw new TimeoutException("Argument probe did not stop");
        }
        Console.WriteLine("Verified SDK arguments with whitespace, quotes, backslashes, Unicode and empty values");
    }
}
