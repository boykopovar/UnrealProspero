using UnrealBuildTool;

public sealed class ApplicationCore_AnyUnreal : ApplicationCore
{
    public ApplicationCore_AnyUnreal(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.AddRange(new[] { "libSceVideoOut", "libSceSystemService", "libSceAgc", "libScePad", "libSceUserService", "libSceSysmodule" });
    }
}
