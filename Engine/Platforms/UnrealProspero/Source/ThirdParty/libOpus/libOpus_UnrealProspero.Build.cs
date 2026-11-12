using System.IO;
using UnrealBuildTool;

public sealed class libOpus_UnrealProspero : libOpus
{
    public libOpus_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicDefinitions.Add("OUTSIDE_SPEEX=1");
        foreach (string name in new[] { "libopus.a", "libresampler.a" })
        {
            string library = Path.Combine(OpusLibPath, target.Platform.ToString(), "lib", name);
            if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit Opus archive: {0}. Build and install Engine/Platforms/UnrealProspero/Build/Opus for this target.", library);
            PublicAdditionalLibraries.Add(library);
            ExternalDependencies.Add(library);
        }
    }
}
