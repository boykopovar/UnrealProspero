using System.IO;
using System.Linq;
using AnyUnreal.Build.Sdk;
using AutomationTool;
using EpicGames.Core;
using UnrealBuildTool;

namespace AnyUnreal.Automation;

public abstract partial class AnyUnrealPlatform
{
    public override void GetFilesToDeployOrStage(ProjectParams parameters, DeploymentContext context)
    {
        if (context.StageTargets.Count == 0) throw new AutomationException("No AnyUnreal build receipts to stage");
        foreach (StageTarget stage in context.StageTargets)
        {
            if (stage.Receipt.Platform != TargetPlatformType) throw new AutomationException("Receipt platform does not match staging platform");
            SdkInstallation sdk = AnyUnrealReceipt.Read(stage.Receipt.AdditionalProperties);
            sdk.ValidateTarget(target);
            context.StageBuildProductsFromReceipt(stage.Receipt, true, parameters.bTreatNonShippingBinariesAsDebugFiles);
            if (target == Target.Ps5) continue;
            foreach (BuildProduct executable in stage.Receipt.BuildProducts.Where(product => product.Type == BuildProductType.Executable))
            {
                string libraries = SdkInstallation.RequireDirectory(Path.Combine(executable.Path.Directory.FullName, "libs"));
                foreach (string file in Directory.EnumerateFiles(libraries, "*", SearchOption.AllDirectories)) context.StageFile(StagedFileType.NonUFS, new FileReference(file));
            }
        }
        string app0 = SdkInstallation.RequireDirectory(Path.Combine(parameters.RawProjectPath.Directory.FullName, "Build/AnyUnreal/app0"));
        SdkInstallation.RequireFile(Path.Combine(app0, "sce_sys/param.json"));
        foreach (string file in Directory.EnumerateFiles(app0, "*", SearchOption.AllDirectories))
        {
            string relative = Path.GetRelativePath(app0, file);
            string destination = target == Target.Ps5 ? relative : Path.Combine("app0", relative);
            context.StageFile(StagedFileType.NonUFS, new FileReference(file), new StagedFileReference(destination));
        }
    }
}
