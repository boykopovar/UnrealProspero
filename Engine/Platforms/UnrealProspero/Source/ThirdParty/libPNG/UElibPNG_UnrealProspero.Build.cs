using System.IO;
using UnrealBuildTool;

public sealed class UElibPNG_UnrealProspero : UElibPNG
{
    public UElibPNG_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        AddEngineThirdPartyPrivateStaticDependencies(target, "zlib");
        string library = Path.Combine(LibPNGPath, target.Platform.ToString(), "libpng.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit libPNG archive: {0}. Build Engine/Platforms/UnrealProspero/Build/LibPng for this target.", library);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
