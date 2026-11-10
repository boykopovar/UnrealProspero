using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

internal static class ExitChecks
{
    public static async Task RunAsync(string executable)
    {
        foreach ((string argument, int expected) in new[] { ("default", 1), ("0", 0), ("73", 73), ("255", 255) })
        {
            ProcessStartInfo info = new(executable)
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = Path.GetDirectoryName(executable)!
            };
            info.ArgumentList.Add("--exit-probe");
            info.ArgumentList.Add(argument);
            using Process process = Process.Start(info) ?? throw new InvalidOperationException("Could not start exit probe");
            try
            {
                using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(10));
                await process.WaitForExitAsync(timeout.Token);
                if (process.ExitCode != expected) throw new InvalidOperationException($"Exit probe returned {process.ExitCode}, expected {expected}");
            }
            finally
            {
                if (!process.HasExited) process.Kill(true);
                if (!process.WaitForExit(10000)) throw new TimeoutException("Exit probe did not stop");
            }
        }
        Console.WriteLine("Verified forced exit status and suppression of exit handlers");
    }
}
