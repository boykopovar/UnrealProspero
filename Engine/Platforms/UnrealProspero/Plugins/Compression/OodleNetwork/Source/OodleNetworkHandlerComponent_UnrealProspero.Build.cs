using System.IO;
using UnrealBuildTool;

public sealed class OodleNetworkHandlerComponent_UnrealProspero : OodleNetworkHandlerComponent
{
    protected override string ReleaseLibraryName => "liboo2net.a";
    protected override string DebugLibraryName => "liboo2net_debug.a";
    protected override string LibDirectory => Path.Combine(base.LibDirectory, Target.Platform.ToString());

    public OodleNetworkHandlerComponent_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyWin64 && target.Platform != UnrealTargetPlatform.AnyLinux && target.Platform != UnrealTargetPlatform.AnyPS5) throw new BuildException("Oodle Network requires an UnrealProspero target");
        PublicDependencyModuleNames.Add("UnrealProsperoOodleRuntime");
        foreach (string library in PublicAdditionalLibraries)
        {
            if (!File.Exists(library)) throw new BuildException("Missing Oodle Network library for {0}: {1}. Prepare the Oodle Network archive for the AnyDevKit target.", target.Platform, library);
            ExternalDependencies.Add(library);
        }
    }
}
