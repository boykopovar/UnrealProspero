#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

class FUnrealProsperoTargetPlatformCommonModule final : public FDefaultModuleImpl
{
public:
    void StartupModule() override
    {
#if WITH_EDITOR
        FString commandlet;
        if (FParse::Value(FCommandLine::Get(), TEXT("run="), commandlet) && commandlet.Equals(TEXT("UnrealProsperoShaderCooker"), ESearchCase::IgnoreCase)) FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("UnrealProsperoShaderCooker"));
#endif
    }
};

IMPLEMENT_MODULE(FUnrealProsperoTargetPlatformCommonModule, UnrealProsperoTargetPlatformCommon)
