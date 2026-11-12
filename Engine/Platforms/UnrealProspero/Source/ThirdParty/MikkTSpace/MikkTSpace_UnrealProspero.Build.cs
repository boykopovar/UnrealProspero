using System.IO;
using UnrealBuildTool;

public sealed class MikkTSpace_UnrealProspero : MikkTSpace
{
    public MikkTSpace_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        string library = Path.Combine(PlatformModuleDirectory, "lib", target.Platform.ToString(), "libMikkTSpace.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit MikkTSpace archive: {0}. Build Engine/Platforms/UnrealProspero/Build/MikkTSpace for this target.", library);
        if (!PublicDefinitions.Remove("WITH_MIKKTSPACE=0")) throw new BuildException("Unexpected base MikkTSpace configuration for {0}.", target.Platform);
        PublicDefinitions.Add("WITH_MIKKTSPACE=1");
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
