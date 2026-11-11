using UnrealBuildTool;

public sealed class ApplicationCore_UnrealProspero : ApplicationCore
{
    public ApplicationCore_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.AddRange(new[] { "libSceVideoOut", "libSceSystemService", "libSceAgc", "libScePad", "libSceUserService", "libSceSysmodule" });
    }
}
