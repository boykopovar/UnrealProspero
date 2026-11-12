using System.IO;
using UnrealBuildTool;

public sealed class BLAKE3_UnrealProspero : BLAKE3
{
    public BLAKE3_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        string library = Path.Combine(LibraryPath, target.Platform.ToString(), "libBLAKE3.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit BLAKE3 archive: {0}. Build Engine/Platforms/UnrealProspero/Build/Blake3 for this target.", library);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
