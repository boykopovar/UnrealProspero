using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using UnrealProspero.Build.Execution;
using UnrealProspero.Build.Sdk;

namespace UnrealProspero.Build.Packaging;

public sealed class Ps5Package
{
    public string ProjectDirectory { get; }
    public string InputElf { get; }
    public string App0 { get; }
    public string Output { get; }
    public string Workspace { get; }

    public Ps5Package(string projectDirectory, string inputElf, string app0, string output, string workspace)
    {
        ProjectDirectory = SdkInstallation.RequireDirectory(Path.GetFullPath(projectDirectory));
        InputElf = SdkInstallation.RequireFile(Path.GetFullPath(inputElf));
        App0 = SdkInstallation.RequireDirectory(Path.GetFullPath(app0));
        Output = Path.GetFullPath(output);
        Workspace = Path.GetFullPath(workspace);
        SdkInstallation.RequireFile(Path.Combine(App0, "sce_sys", "param.json"));
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(SdkInstallation.RequireFile(Path.Combine(ProjectDirectory, "adk.json"))));
        if (document.RootElement.GetProperty("schemaVersion").GetInt32() != 1) throw new InvalidDataException("Unsupported ADK project schema");
        document.RootElement.GetProperty("targets").GetProperty("ps5");
        if (!Output.EndsWith(".ffpfsc", StringComparison.OrdinalIgnoreCase)) throw new ArgumentException("Expected an ffpfsc output", nameof(output));
        if (File.Exists(Output) || Directory.Exists(Output)) throw new IOException("Package output already exists: " + Output);
        if (File.Exists(Workspace) || Directory.Exists(Workspace)) throw new IOException("Package workspace already exists: " + Workspace);
        using FileStream input = File.OpenRead(InputElf);
        Span<byte> magic = stackalloc byte[4];
        input.ReadExactly(magic);
        if (magic[0] != 0x7f || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') throw new InvalidDataException("Package input is not ELF");
    }

    public ToolCommand CreateCommand(SdkInstallation sdk)
    {
        sdk.ValidateTarget(Target.Ps5);
        List<string> arguments = new()
        {
            "package", "--target", "ps5", "--project", ProjectDirectory,
            "--input", InputElf, "--app0", App0,
            "--param", Path.Combine(App0, "sce_sys", "param.json"),
            "--output", Output, "--workspace", Workspace, "--format", "ffpfsc"
        };
        return new ToolCommand(sdk.Executable, ProjectDirectory, arguments);
    }

    public async Task BuildAsync(SdkInstallation sdk, CancellationToken cancellationToken)
    {
        await CreateCommand(sdk).ExecuteAsync(cancellationToken).ConfigureAwait(false);
        FileInfo package = new(Output);
        if (!package.Exists || package.Length == 0) throw new InvalidDataException("ADK did not produce the requested package");
    }
}
