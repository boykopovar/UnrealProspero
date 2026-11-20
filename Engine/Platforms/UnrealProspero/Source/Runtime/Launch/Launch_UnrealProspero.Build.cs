using UnrealBuildTool;

public sealed class Launch_UnrealProspero : Launch
{
    public Launch_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PrivateDefinitions.Add("PLATFORM_USE_GENERIC_LAUNCH_IMPLEMENTATION=1");
    }
}
