using System.IO;
using UnrealBuildTool;

public sealed class HarfBuzz_UnrealProspero : HarfBuzz
{
    protected override string HarfBuzzVersion => "harfbuzz-2.4.0";

    public HarfBuzz_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (!target.bCompileFreeType || !target.bCompileICU) return;
        AddEngineThirdPartyPrivateStaticDependencies(target, "FreeType2", "ICU");
        string library = Path.Combine(PlatformModuleDirectory, HarfBuzzVersion, "lib", target.Platform.ToString(), "libharfbuzz.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit HarfBuzz archive: {0}. Build Engine/Platforms/UnrealProspero/Build/HarfBuzz for this target.", library);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
