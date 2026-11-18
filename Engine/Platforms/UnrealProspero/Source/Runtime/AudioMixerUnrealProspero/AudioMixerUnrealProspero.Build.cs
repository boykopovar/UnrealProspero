using UnrealBuildTool;

public sealed class AudioMixerUnrealProspero : ModuleRules
{
    public AudioMixerUnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        bEnableExceptions = true;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "Engine", "AudioMixer", "AudioMixerCore" });
        PublicSystemLibraries.Add("libSceAudioOut");
    }
}
