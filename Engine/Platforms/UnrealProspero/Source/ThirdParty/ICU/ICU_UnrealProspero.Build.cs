using System.IO;
using UnrealBuildTool;

public sealed class ICU_UnrealProspero : ICU
{
    protected override string ICUVersion => ICU64VersionString;

    public ICU_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        string library = Path.Combine(PlatformModuleDirectory, ICUVersion, "lib", target.Platform.ToString(), "libicu.a");
        if (!File.Exists(library)) throw new BuildException("Missing AnyDevKit ICU archive: {0}. Build Engine/Platforms/UnrealProspero/Build/ICU for this target.", library);
        PublicAdditionalLibraries.Add(library);
        ExternalDependencies.Add(library);
    }
}
