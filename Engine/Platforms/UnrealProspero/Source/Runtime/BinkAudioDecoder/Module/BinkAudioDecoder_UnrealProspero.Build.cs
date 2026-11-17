using System.IO;
using UnrealBuildTool;

public sealed class BinkAudioDecoder_UnrealProspero : BinkAudioDecoder
{
    public BinkAudioDecoder_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
    }

    protected override string GetLibrary(ReadOnlyTargetRules target)
    {
        string archive = Path.Combine(GetModuleDirectoryForSubClass(GetType()).FullName, "..", "SDK", "BinkAudio", "Lib", "libbinka_ue_decode_" + target.Platform + ".a");
        if (!File.Exists(archive)) throw new BuildException("Missing AnyDevKit Bink Audio archive: {0}. Build and install Engine/Platforms/UnrealProspero/Build/BinkAudio for this target.", archive);
        ExternalDependencies.Add(archive);
        return archive;
    }
}
