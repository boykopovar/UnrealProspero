using System.IO;
using UnrealBuildTool;

public sealed class VorbisFile_UnrealProspero : VorbisFile
{
    public VorbisFile_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        AddEngineThirdPartyPrivateStaticDependencies(target, "Vorbis");
        foreach (string name in new[] { "libvorbisfile.a", "libvorbisenc.a" })
        {
            string library = Path.Combine(PlatformModuleDirectory, "libvorbis-1.3.2", target.Platform.ToString(), "lib", name);
            if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit audio archive: {0}. Install Engine/Platforms/UnrealProspero/Build/OggVorbis for this target.", library);
            PublicAdditionalLibraries.Add(library);
            ExternalDependencies.Add(library);
        }
    }
}
