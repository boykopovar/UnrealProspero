using System.IO;
using UnrealBuildTool;

public sealed class zlib_UnrealProspero : zlib
{
    public zlib_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemIncludePaths.Remove(Path.Combine(VersionPath, "include"));
        PublicSystemIncludePaths.Remove(Path.Combine(VersionPath, "include", "minizip"));
        PublicSystemIncludePaths.Add(VersionPath);
        string library = Path.Combine(PlatformModuleDirectory, Version, "lib", target.Platform.ToString(), "libz.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit zlib archive: {0}. Build Engine/Platforms/UnrealProspero/Build/Zlib for this target.", library);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
