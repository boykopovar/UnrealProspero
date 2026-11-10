using UnrealBuildTool;

public sealed class ApplicationCore_AnyUnreal : ApplicationCore
{
    public ApplicationCore_AnyUnreal(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.Add("libSceVideoOut");
    }
}
