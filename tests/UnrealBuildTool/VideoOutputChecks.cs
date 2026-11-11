using System.IO;
using System.Threading;
using System.Threading.Tasks;
using UnrealProspero.Build.Execution;
using UnrealProspero.Build.Sdk;

internal static class VideoOutputChecks
{
    public static async Task PrepareAsync(SdkInstallation sdk, string outputDirectory, string executableDirectory)
    {
        string project = Path.Combine(outputDirectory, "video-project");
        await new ToolCommand(sdk.Executable, outputDirectory, new[]
        {
            "init", "VideoOutputProbe", "--deps", sdk.Dependencies,
            "--directory", project, "--title-id", "FAKE26111"
        }).ExecuteAsync(CancellationToken.None);
        string metadata = Path.Combine(executableDirectory, "app0", "sce_sys");
        Directory.CreateDirectory(metadata);
        File.Copy(SdkInstallation.RequireFile(Path.Combine(project, "app0", "sce_sys", "param.json")), Path.Combine(metadata, "param.json"));
    }
}
