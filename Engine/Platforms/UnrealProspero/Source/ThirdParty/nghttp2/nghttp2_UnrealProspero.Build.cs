using System.IO;
using UnrealBuildTool;

public sealed class nghttp2_UnrealProspero : nghttp2
{
    public nghttp2_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyPS5) return;
        string directory = Path.Combine(PlatformModuleDirectory, target.Platform.ToString());
        string include = Path.Combine(directory, "include");
        string library = Path.Combine(directory, "lib", "libnghttp2.a");
        if (!File.Exists(library) || !File.Exists(Path.Combine(include, "nghttp2", "nghttp2ver.h")))
            throw new BuildException("Missing AnyDevKit nghttp2 installation: {0}. Build Engine/Platforms/UnrealProspero/Build/Nghttp2 for this target.", directory);
        PublicSystemIncludePaths.Add(include);
        PublicAdditionalLibraries.Add(library);
        PublicDefinitions.Add("NGHTTP2_STATICLIB=1");
        ExternalDependencies.Add(library);
    }
}
