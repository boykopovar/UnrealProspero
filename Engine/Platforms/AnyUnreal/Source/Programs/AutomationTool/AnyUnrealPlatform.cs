using AnyUnreal.Build.Sdk;
using AutomationTool;
using UnrealBuildTool;

namespace AnyUnreal.Automation;

public abstract partial class AnyUnrealPlatform : Platform
{
    private readonly Target target;

    protected AnyUnrealPlatform(UnrealTargetPlatform platform, Target target) : base(platform)
    {
        this.target = target;
    }

    protected override string GetPlatformExeExtension() => target == Target.Windows ? ".exe" : target == Target.Ps5 ? ".elf" : "";
}

public sealed class AnyWin64Platform : AnyUnrealPlatform
{
    public AnyWin64Platform() : base(UnrealTargetPlatform.AnyWin64, Target.Windows) { }
}

public sealed class AnyLinuxPlatform : AnyUnrealPlatform
{
    public AnyLinuxPlatform() : base(UnrealTargetPlatform.AnyLinux, Target.Linux) { }
}

public sealed class AnyPS5Platform : AnyUnrealPlatform
{
    public AnyPS5Platform() : base(UnrealTargetPlatform.AnyPS5, Target.Ps5) { }
}
