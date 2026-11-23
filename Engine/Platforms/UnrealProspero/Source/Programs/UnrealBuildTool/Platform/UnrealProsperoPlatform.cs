using System;
using System.IO;
using System.Threading;
using UnrealProspero.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildBase;

namespace UnrealBuildTool;

internal sealed class UnrealProsperoPlatform : UEBuildPlatform
{
    private readonly UnrealProsperoSettings settings;
    private readonly UnrealProsperoPlatformSdk sdk;
    private readonly Target target;

    public UnrealProsperoPlatform(UnrealTargetPlatform platform, UnrealProsperoSettings settings, ILogger logger) : this(platform, settings, new UnrealProsperoPlatformSdk(logger, UnrealProsperoPlatformNames.GetTarget(platform), settings), logger) { }

    private UnrealProsperoPlatform(UnrealTargetPlatform platform, UnrealProsperoSettings settings, UnrealProsperoPlatformSdk sdk, ILogger logger) : base(platform, sdk, new UnrealArchitectureConfig(UnrealArch.X64), logger)
    {
        this.settings = settings;
        this.sdk = sdk;
        target = UnrealProsperoPlatformNames.GetTarget(platform);
    }

    public override string GetPlatformName() => "UnrealProspero";
    public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform platform) => true;
    public override bool ShouldCreateDebugInfo(ReadOnlyTargetRules rules) => true;
    public override string[] GetDebugInfoExtensions(ReadOnlyTargetRules rules, UEBuildBinaryType binaryType) => Array.Empty<string>();

    public override void ModifyModuleRulesForOtherPlatform(string moduleName, ModuleRules rules, ReadOnlyTargetRules targetRules)
    {
        if (targetRules.Platform == Platform) return;
        if (targetRules.Platform != UnrealTargetPlatform.Win64 && targetRules.Platform != UnrealTargetPlatform.Linux) return;
        if (targetRules.Type != TargetType.Editor && targetRules.Type != TargetType.Program) return;
        if (targetRules.bBuildRequiresCookedData) return;
        if (targetRules.Type == TargetType.Program && !targetRules.bNeedsExtraShaderFormats && !targetRules.bForceBuildShaderFormats && !targetRules.bForceBuildTargetPlatforms) return;
        if (settings.IsConfigured) AddPlatformStaticShaders(targetRules, rules, new[] { settings.GetShaderPlatform(target) });
        if ((moduleName == "Engine" && targetRules.bBuildDeveloperTools) || (moduleName == "TargetPlatform" && targetRules.bForceBuildTargetPlatforms))
        {
            string settingsModule = Platform + "TargetPlatformSettings";
            if (!rules.DynamicallyLoadedModuleNames.Contains(settingsModule)) rules.DynamicallyLoadedModuleNames.Add(settingsModule);
            if (settings.IsConfigured && sdk.HasRequiredManualSDK() == SDKStatus.Valid)
            {
                foreach (string suffix in new[] { "TargetPlatform", "TargetPlatformControls" })
                {
                    string platformModule = Platform + suffix;
                    if (!rules.DynamicallyLoadedModuleNames.Contains(platformModule)) rules.DynamicallyLoadedModuleNames.Add(platformModule);
                }
            }
        }
        if (!settings.IsConfigured || moduleName != "TargetPlatform") return;
        if (!targetRules.bForceBuildShaderFormats && !targetRules.bNeedsExtraShaderFormats && targetRules.Type == TargetType.Program) return;
        if (!rules.DynamicallyLoadedModuleNames.Contains("UnrealProsperoShaderFormat")) rules.DynamicallyLoadedModuleNames.Add("UnrealProsperoShaderFormat");
    }

    public override void ModifyModuleRulesForActivePlatform(string moduleName, ModuleRules rules, ReadOnlyTargetRules targetRules)
    {
        AddPlatformStaticShaders(targetRules, rules, new[] { settings.GetShaderPlatform(target) });
        if (moduleName == "Engine" && !rules.DynamicallyLoadedModuleNames.Contains("UnrealProsperoPlatformFeatures")) rules.DynamicallyLoadedModuleNames.Add("UnrealProsperoPlatformFeatures");
        if (target == Target.Ps5 && moduleName == "Engine") rules.DynamicallyLoadedModuleNames.Add("AudioMixerUnrealProspero");
        if (moduleName == "RHI")
        {
            if (!rules.DynamicallyLoadedModuleNames.Contains("UnrealProsperoRHI")) rules.DynamicallyLoadedModuleNames.Add("UnrealProsperoRHI");
            rules.FilesToGenerate.Add("UnrealProsperoRHIShaderPlatform.h", new[]
            {
                "#ifndef UNREALPROSPERO_GENERATED_RHI_SHADER_PLATFORM_H",
                "#define UNREALPROSPERO_GENERATED_RHI_SHADER_PLATFORM_H",
                "#include \"" + Platform + "RHIShaderPlatform.h\"",
                "#endif"
            });
        }
    }

    public override void ResetTarget(TargetRules rules)
    {
        rules.bForceEnableExceptions = true;
        rules.bUsePDBFiles = false;
    }

    public override void ValidateTarget(TargetRules rules)
    {
        base.ValidateTarget(rules);
        sdk.RequireInstallation();
        if (rules.Type == TargetType.Editor) throw new BuildException("Use the native Unreal Editor to cook for UnrealProspero");
        if (rules.LinkType != TargetLinkType.Monolithic) throw new BuildException("UnrealProspero requires monolithic application builds");
        if (rules.Architecture != UnrealArch.X64) throw new BuildException("UnrealProspero requires x86-64");
        if (rules.bUseAutoRTFMCompiler || rules.bUseAutoRTFMVerifier) throw new BuildException("The SDK does not provide AutoRTFM");
    }

    public override void SetUpEnvironment(ReadOnlyTargetRules rules, CppCompileEnvironment compileEnvironment, LinkEnvironment linkEnvironment)
    {
        compileEnvironment.Definitions.Add("PLATFORM_UNREALPROSPERO=1");
        compileEnvironment.Definitions.Add("OVERRIDE_PLATFORM_HEADER_NAME=UnrealProspero");
        compileEnvironment.Definitions.Add("UNREALPROSPERO_TARGET_" + SdkInstallation.GetTargetName(target).ToUpperInvariant() + "=1");
        compileEnvironment.bEnableExceptions = true;
        linkEnvironment.SystemLibraries.Add("libc");
        linkEnvironment.SystemLibraries.Add("libkernel");
    }

    public override string GetBinaryExtension(UEBuildBinaryType binaryType) => binaryType switch
    {
        UEBuildBinaryType.Executable => target == Target.Windows ? ".exe" : target == Target.Ps5 ? ".elf" : "",
        UEBuildBinaryType.StaticLibrary => ".a",
        UEBuildBinaryType.DynamicLinkLibrary => ".prx",
        _ => throw new BuildException("Unsupported UnrealProspero binary type: " + binaryType)
    };

    public override bool IsBuildProduct(string fileName, string[] namePrefixes, string[] nameSuffixes) => IsBuildProductName(fileName, namePrefixes, nameSuffixes, GetBinaryExtension(UEBuildBinaryType.Executable)) || IsBuildProductName(fileName, namePrefixes, nameSuffixes, ".a") || IsBuildProductName(fileName, namePrefixes, nameSuffixes, ".map");

    public override UEToolChain CreateToolChain(ReadOnlyTargetRules rules)
    {
        SdkInstallation installation = sdk.RequireInstallation();
        string buildRoot = (rules.ProjectFile?.Directory ?? Unreal.EngineDirectory).FullName;
        string profileDirectory = Path.Combine(buildRoot, "Intermediate/Build/UnrealProspero", Platform.ToString(), rules.Name, "Profile", Guid.NewGuid().ToString("N"));
        string sourceDirectory = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/UnrealProspero/Build");
        RuntimeProfile profile = RuntimeProfile.ConfigureAsync(installation, target, settings.RequireCMake(), Path.Combine(sourceDirectory, "Toolchain"), profileDirectory, CancellationToken.None).GetAwaiter().GetResult();
        ClangToolChainOptions options = ClangToolChainOptions.None;
        if (rules.bAllowLTCG) options |= ClangToolChainOptions.EnableLinkTimeOptimization;
        if (rules.bPreferThinLTO) options |= ClangToolChainOptions.EnableThinLTO;
        return new UnrealProsperoToolChain(installation, profile, target, settings.RequireCMake(), Path.Combine(sourceDirectory, "Linker"), options, Logger);
    }

    public override void Deploy(TargetReceipt receipt) => throw new BuildException("UnrealProspero deployment requires the AutomationTool packaging integration");
}
