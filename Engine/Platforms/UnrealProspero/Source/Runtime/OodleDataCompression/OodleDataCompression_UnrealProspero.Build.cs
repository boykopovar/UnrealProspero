using System.IO;
using UnrealBuildTool;

public sealed class OodleDataCompression_UnrealProspero : OodleDataCompression
{
    protected override string ReleaseLibraryName => "liboo2core.a";
    protected override string DebugLibraryName => "liboo2core_debug.a";
    protected override string LibDirectory => Path.Combine(base.LibDirectory, Target.Platform.ToString());

    public OodleDataCompression_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyWin64 && target.Platform != UnrealTargetPlatform.AnyLinux && target.Platform != UnrealTargetPlatform.AnyPS5) throw new BuildException("Oodle requires an UnrealProspero target");
        foreach (string library in PublicAdditionalLibraries)
        {
            if (!File.Exists(library)) throw new BuildException("Missing Oodle library for {0}: {1}. Build Oodle with the AnyDevKit target toolchain; native Windows and Linux archives require a different runtime.", target.Platform, library);
            ExternalDependencies.Add(library);
        }
    }
}
