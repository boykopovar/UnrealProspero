using System.IO;
using UnrealBuildTool;

public sealed class UEOgg_UnrealProspero : UEOgg
{
    protected override string OggIncPath => Path.Combine(PlatformModuleDirectory, OggVersion, Target.Platform.ToString(), "include");

    public UEOgg_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (!File.Exists(Path.Combine(OggIncPath, "ogg", "config_types.h"))) throw new BuildException("Missing AnyDevKit Ogg headers: {0}. Install Engine/Platforms/UnrealProspero/Build/OggVorbis for this target.", OggIncPath);
        foreach (string name in new[] { "libogg.a" })
        {
            string library = Path.Combine(PlatformModuleDirectory, "libogg-1.2.2", target.Platform.ToString(), "lib", name);
            if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit audio archive: {0}. Install Engine/Platforms/UnrealProspero/Build/OggVorbis for this target.", library);
            PublicAdditionalLibraries.Add(library);
            ExternalDependencies.Add(library);
        }
    }
}
