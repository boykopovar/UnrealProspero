using UnrealBuildTool;

public sealed class HTTP_UnrealProspero : HTTP
{
    public HTTP_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.AddRange(new[] { "libSceNet", "libSceSsl", "libSceHttp2" });
    }
}
