using System.IO;
using UnrealBuildTool;

public sealed class FreeType2_UnrealProspero : FreeType2
{
    protected override string FreeType2Version => "FreeType2-2.14.1";
    private string Installation => Path.Combine(PlatformModuleDirectory, FreeType2Version, Target.Platform.ToString());
    protected override string FreeType2IncPath => Path.Combine(Installation, "include", "freetype2");

    public FreeType2_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        string library = Path.Combine(Installation, "lib", "libfreetype.a");
        if (!File.Exists(library) || !File.Exists(Path.Combine(FreeType2IncPath, "ft2build.h"))) throw new BuildException("Missing AnyDevKit FreeType installation: {0}. Build and install Engine/Platforms/UnrealProspero/Build/FreeType for this target.", Installation);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
