using System;
using System.IO;
using System.Threading;
using AnyUnreal.Build.Sdk;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildBase;

namespace UnrealBuildTool;

internal sealed class AnyUnrealPlatform : UEBuildPlatform
{
    private readonly AnyUnrealSettings settings;
    private readonly AnyUnrealPlatformSdk sdk;
    private readonly Target target;

    public AnyUnrealPlatform(UnrealTargetPlatform platform, AnyUnrealSettings settings, ILogger logger) : this(platform, settings, new AnyUnrealPlatformSdk(logger, AnyUnrealPlatformNames.GetTarget(platform), settings.IsConfigured ? settings.CreateInstallation() : null), logger) { }

    private AnyUnrealPlatform(UnrealTargetPlatform platform, AnyUnrealSettings settings, AnyUnrealPlatformSdk sdk, ILogger logger) : base(platform, sdk, new UnrealArchitectureConfig(UnrealArch.X64), logger)
    {
        this.settings = settings;
        this.sdk = sdk;
        target = AnyUnrealPlatformNames.GetTarget(platform);
    }

    public override string GetPlatformName() => "AnyUnreal";
    public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform platform) => true;
    public override bool ShouldCreateDebugInfo(ReadOnlyTargetRules rules) => true;
    public override string[] GetDebugInfoExtensions(ReadOnlyTargetRules rules, UEBuildBinaryType binaryType) => Array.Empty<string>();

    public override void ModifyModuleRulesForOtherPlatform(string moduleName, ModuleRules rules, ReadOnlyTargetRules targetRules)
    {
        if (!settings.IsConfigured || moduleName != "TargetPlatform") return;
        if (targetRules.Platform != UnrealTargetPlatform.Win64 && targetRules.Platform != UnrealTargetPlatform.Linux) return;
        if (!targetRules.bForceBuildShaderFormats && !targetRules.bNeedsExtraShaderFormats) return;
        if (!rules.DynamicallyLoadedModuleNames.Contains("AnyUnrealShaderFormat")) rules.DynamicallyLoadedModuleNames.Add("AnyUnrealShaderFormat");
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
        if (rules.Type == TargetType.Editor) throw new BuildException("Use the native Unreal Editor to cook for AnyUnreal");
        if (rules.LinkType != TargetLinkType.Monolithic) throw new BuildException("AnyUnreal requires monolithic application builds");
        if (rules.Architecture != UnrealArch.X64) throw new BuildException("AnyUnreal requires x86-64");
        if (rules.bUseAutoRTFMCompiler || rules.bUseAutoRTFMVerifier) throw new BuildException("The SDK does not provide AutoRTFM");
    }

    public override void SetUpEnvironment(ReadOnlyTargetRules rules, CppCompileEnvironment compileEnvironment, LinkEnvironment linkEnvironment)
    {
        compileEnvironment.Definitions.Add("PLATFORM_ANYUNREAL=1");
        compileEnvironment.Definitions.Add("OVERRIDE_PLATFORM_HEADER_NAME=AnyUnreal");
        compileEnvironment.Definitions.Add("ANYUNREAL_TARGET_" + SdkInstallation.GetTargetName(target).ToUpperInvariant() + "=1");
        compileEnvironment.bEnableExceptions = true;
        linkEnvironment.SystemLibraries.Add("libc");
        linkEnvironment.SystemLibraries.Add("libkernel");
    }

    public override string GetBinaryExtension(UEBuildBinaryType binaryType) => binaryType switch
    {
        UEBuildBinaryType.Executable => target == Target.Windows ? ".exe" : target == Target.Ps5 ? ".elf" : "",
        UEBuildBinaryType.StaticLibrary => ".a",
        _ => throw new BuildException("Unsupported AnyUnreal binary type: " + binaryType)
    };

    public override bool IsBuildProduct(string fileName, string[] namePrefixes, string[] nameSuffixes) => IsBuildProductName(fileName, namePrefixes, nameSuffixes, GetBinaryExtension(UEBuildBinaryType.Executable)) || IsBuildProductName(fileName, namePrefixes, nameSuffixes, ".a") || IsBuildProductName(fileName, namePrefixes, nameSuffixes, ".map");

    public override UEToolChain CreateToolChain(ReadOnlyTargetRules rules)
    {
        SdkInstallation installation = sdk.RequireInstallation();
        string buildRoot = (rules.ProjectFile?.Directory ?? Unreal.EngineDirectory).FullName;
        string profileDirectory = Path.Combine(buildRoot, "Intermediate/Build/AnyUnreal", Platform.ToString(), rules.Name, "Profile", Guid.NewGuid().ToString("N"));
        string sourceDirectory = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Build");
        RuntimeProfile profile = RuntimeProfile.ConfigureAsync(installation, target, settings.RequireCMake(), Path.Combine(sourceDirectory, "Toolchain"), profileDirectory, CancellationToken.None).GetAwaiter().GetResult();
        ClangToolChainOptions options = ClangToolChainOptions.None;
        if (rules.bAllowLTCG) options |= ClangToolChainOptions.EnableLinkTimeOptimization;
        if (rules.bPreferThinLTO) options |= ClangToolChainOptions.EnableThinLTO;
        return new AnyUnrealToolChain(installation, profile, target, settings.RequireCMake(), Path.Combine(sourceDirectory, "Linker"), options, Logger);
    }

    public override void Deploy(TargetReceipt receipt) => throw new BuildException("AnyUnreal deployment requires the AutomationTool packaging integration");
}
