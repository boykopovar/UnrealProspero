using UnrealProspero.Build.Sdk;
using AutomationTool;
using UnrealBuildTool;

namespace UnrealProspero.Automation;

public abstract partial class UnrealProsperoPlatform : Platform
{
    private readonly Target target;

    protected UnrealProsperoPlatform(UnrealTargetPlatform platform, Target target) : base(platform)
    {
        this.target = target;
    }

    protected override string GetPlatformExeExtension() => target == Target.Windows ? ".exe" : target == Target.Ps5 ? ".elf" : "";
}

public sealed class AnyPS5Platform : UnrealProsperoPlatform
{
    public AnyPS5Platform() : base(UnrealTargetPlatform.AnyPS5, Target.Ps5) { }
}
