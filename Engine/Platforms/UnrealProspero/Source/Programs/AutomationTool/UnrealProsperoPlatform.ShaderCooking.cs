using System.IO;
using AutomationTool;
using EpicGames.Core;
using UnrealBuildTool;
using UnrealProspero.Build.Sdk;

namespace UnrealProspero.Automation;

public abstract partial class UnrealProsperoPlatform
{
    private void finalizeShaderArchives(ProjectParams parameters, DeploymentContext context)
    {
        if (target != Target.Ps5) return;
        if (context.StageTargets.Count != 1) throw new AutomationException("PS5 shader finalization requires exactly one target receipt");
        ConfigHierarchy config = ConfigCache.ReadHierarchy(ConfigHierarchyType.Game, parameters.RawProjectPath.Directory, context.StageTargetPlatform.IniPlatformType, context.CustomConfig);
        if (!config.GetBool("/Script/UnrealEd.ProjectPackagingSettings", "bShareMaterialShaderCode", out bool shared) || !shared) throw new AutomationException("UnrealProspero requires shared shader code for offline pipeline cooking");
        if (context.MetadataDir == null) throw new AutomationException("Cooked shader metadata was not initialized before staging");
        SdkInstallation sdk = UnrealProsperoReceipt.Read(context.StageTargets[0].Receipt.AdditionalProperties);
        sdk.ValidateTarget(target);
        string source = SdkInstallation.RequireDirectory(Path.Combine(context.MetadataDir.FullName, "ShaderLibrarySource"));
        string output = SdkInstallation.RequireDirectory(Path.GetFullPath(Path.Combine(context.MetadataDir.FullName, "..", "Content")));
        string work = Path.Combine(parameters.RawProjectPath.Directory.FullName, "Intermediate", "UnrealProspero", "ShaderCook");
        Directory.CreateDirectory(work);
        string arguments = "-Source=" + CommandUtils.MakePathSafeToUseWithCommandLine(source)
            + " -Output=" + CommandUtils.MakePathSafeToUseWithCommandLine(output)
            + " -Compiler=" + CommandUtils.MakePathSafeToUseWithCommandLine(sdk.Executable)
            + " -WorkingRoot=" + CommandUtils.MakePathSafeToUseWithCommandLine(work)
            + " -TargetPlatform=" + context.CookPlatform + " -unattended -nop4 -NullRHI -DisablePython";
        if (parameters.HasDDCGraph) arguments += " -ddc=" + CommandUtils.MakePathSafeToUseWithCommandLine(parameters.DDCGraph);
        CommandUtils.RunCommandlet(parameters.RawProjectPath, parameters.UnrealExe, "UnrealProsperoShaderCooker", arguments);
    }
}
