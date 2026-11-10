using System;
using System.IO;
using System.Linq;
using System.Threading;
using AnyUnreal.Build.Packaging;
using AnyUnreal.Build.Sdk;
using AutomationTool;
using EpicGames.Core;
using UnrealBuildTool;

namespace AnyUnreal.Automation;

public abstract partial class AnyUnrealPlatform
{
    public override void Package(ProjectParams parameters, DeploymentContext context, int workingChangelist)
    {
        SdkInstallation.RequireDirectory(context.StageDirectory.FullName);
        if (context.StageTargets.Count == 0) throw new AutomationException("No AnyUnreal build receipts to package");
        foreach (StageTarget stage in context.StageTargets)
        {
            if (stage.Receipt.Platform != TargetPlatformType) throw new AutomationException("Receipt platform does not match packaging platform");
            AnyUnrealReceipt.Read(stage.Receipt.AdditionalProperties).ValidateTarget(target);
        }
        if (target != Target.Ps5)
        {
            foreach (StageTarget stage in context.StageTargets)
            {
                BuildProduct[] products = stage.Receipt.BuildProducts.Where(product => product.Type == BuildProductType.Executable).ToArray();
                if (products.Length == 0) throw new AutomationException("No executable in AnyUnreal receipt");
                foreach (BuildProduct product in products)
                {
                    string executable = SdkInstallation.RequireFile(Path.Combine(context.StageDirectory.FullName, context.GetStagedFileLocation(product.Path).Name));
                    SdkInstallation.RequireDirectory(Path.Combine(Path.GetDirectoryName(executable)!, "libs"));
                }
            }
            return;
        }
        if (context.StageTargets.Count != 1) throw new AutomationException("A PS5 image requires exactly one target receipt");
        TargetReceipt receipt = context.StageTargets[0].Receipt;
        BuildProduct[] executables = receipt.BuildProducts.Where(product => product.Type == BuildProductType.Executable).ToArray();
        if (executables.Length != 1) throw new AutomationException("A PS5 image requires exactly one executable");
        SdkInstallation sdk = AnyUnrealReceipt.Read(receipt.AdditionalProperties);
        string metadata = Path.Combine(parameters.RawProjectPath.Directory.FullName, "Build/AnyUnreal");
        string work = Path.Combine(parameters.RawProjectPath.Directory.FullName, "Intermediate/AnyUnreal/Packaging", Guid.NewGuid().ToString("N"));
        string app0 = Path.Combine(work, "app0");
        string stagedElf = SdkInstallation.RequireFile(Path.Combine(context.StageDirectory.FullName, context.GetStagedFileLocation(executables[0].Path).Name));
        foreach (string file in Directory.EnumerateFiles(context.StageDirectory.FullName, "*", SearchOption.AllDirectories))
        {
            if (new FileReference(file) == new FileReference(stagedElf)) continue;
            string destination = Path.Combine(app0, Path.GetRelativePath(context.StageDirectory.FullName, file));
            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            File.Copy(file, destination);
        }
        string image = Path.Combine(work, "application.ffpfsc");
        Ps5Package package = new(metadata, stagedElf, app0, image, Path.Combine(work, "package"));
        package.BuildAsync(sdk, CancellationToken.None).GetAwaiter().GetResult();
        File.Move(image, context.StageDirectory.FullName + ".ffpfsc", true);
    }

    public override void GetFilesToArchive(ProjectParams parameters, DeploymentContext context)
    {
        if (target != Target.Ps5)
        {
            base.GetFilesToArchive(parameters, context);
            return;
        }
        string image = SdkInstallation.RequireFile(context.StageDirectory.FullName + ".ffpfsc");
        context.ArchiveFiles(Path.GetDirectoryName(image)!, Path.GetFileName(image), false);
    }
}
