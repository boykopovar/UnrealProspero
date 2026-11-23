using System.IO;
using UnrealBuildTool;

public sealed class OpenSSL_UnrealProspero : OpenSSL
{
    public OpenSSL_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        if (target.Platform != UnrealTargetPlatform.AnyPS5) return;
        string directory = Path.Combine(PlatformModuleDirectory, target.Platform.ToString());
        string include = Path.Combine(directory, "include");
        string ssl = Path.Combine(directory, "lib", "libssl.a");
        string crypto = Path.Combine(directory, "lib", "libcrypto.a");
        if (!File.Exists(ssl) || !File.Exists(crypto) || !File.Exists(Path.Combine(include, "openssl", "ssl.h")))
            throw new BuildException("Missing AnyDevKit OpenSSL installation: {0}. Run Engine/Platforms/UnrealProspero/Build/OpenSsl/Build.sh.", directory);
        PublicSystemIncludePaths.Add(include);
        PublicAdditionalLibraries.Add(ssl);
        PublicAdditionalLibraries.Add(crypto);
        PublicSystemLibraries.Add("libSceRandom");
        ExternalDependencies.Add(ssl);
        ExternalDependencies.Add(crypto);
    }
}
