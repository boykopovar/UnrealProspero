using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

internal static class FileLockChecks
{
    public static async Task RunAsync(string executable)
    {
        string directory = Path.GetDirectoryName(executable)!;
        string path = Path.Combine(directory, "process-lock");
        string marker = Path.Combine(directory, "process-lock-ready");
        if (File.Exists(marker)) throw new IOException("File lock marker already exists");
        using (File.Open(path, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { }
        try
        {
            using Process owner = start(executable, "--hold-file-lock", path, marker);
            try
            {
                Stopwatch deadline = Stopwatch.StartNew();
                while (!File.Exists(marker))
                {
                    if (owner.HasExited) throw new InvalidOperationException("File lock owner exited before acquiring the lock");
                    if (deadline.Elapsed > TimeSpan.FromSeconds(10)) throw new TimeoutException("File lock owner did not acquire the lock");
                    await Task.Delay(10);
                }
                if (await probeAsync(executable, path) != 41) throw new InvalidOperationException("File lock was not exclusive across processes");
            }
            finally
            {
                stop(owner);
            }
            if (await probeAsync(executable, path) != 0) throw new InvalidOperationException("File lock survived owner termination");
        }
        finally
        {
            File.Delete(marker);
            File.Delete(path);
        }
        Console.WriteLine("Verified file lock exclusion and owner termination across processes");
    }

    private static Process start(string executable, params string[] arguments)
    {
        ProcessStartInfo info = new(executable)
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            WorkingDirectory = Path.GetDirectoryName(executable)!
        };
        foreach (string argument in arguments) info.ArgumentList.Add(argument);
        return Process.Start(info) ?? throw new InvalidOperationException("Could not start file lock probe");
    }

    private static async Task<int> probeAsync(string executable, string path)
    {
        using Process process = start(executable, "--try-file-lock", path);
        try
        {
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(10));
            await process.WaitForExitAsync(timeout.Token);
            return process.ExitCode;
        }
        finally
        {
            stop(process);
        }
    }

    private static void stop(Process process)
    {
        if (!process.HasExited) process.Kill(true);
        if (!process.WaitForExit(10000)) throw new TimeoutException("File lock probe did not exit");
    }
}
