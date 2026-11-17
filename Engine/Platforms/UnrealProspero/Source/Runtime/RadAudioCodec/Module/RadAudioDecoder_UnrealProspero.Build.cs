using System.IO;
using UnrealBuildTool;

public sealed class RadAudioDecoder_UnrealProspero : RadAudioDecoder
{
    public RadAudioDecoder_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        string archive = Path.Combine(GetModuleDirectoryForSubClass(GetType()).FullName, "..", "SDK", "Lib", "libradaudio_decoder_" + target.Platform + ".a");
        if (!File.Exists(archive)) throw new BuildException("Missing AnyDevKit RAD Audio archive: {0}. Build and install Engine/Platforms/UnrealProspero/Build/RadAudio for this target.", archive);
        ExternalDependencies.Add(archive);
    }

    protected override string[] GetRadAudioPlatformString(ReadOnlyTargetRules target, out bool isWindowsLib)
    {
        isWindowsLib = false;
        return new[] { target.Platform.ToString() };
    }
}
