using UnrealBuildTool;

public sealed class Slate_UnrealProspero : Slate
{
    public Slate_UnrealProspero(ReadOnlyTargetRules target) : base(target)
    {
        PublicSystemLibraries.Add("libSceImeDialog");
    }
}
