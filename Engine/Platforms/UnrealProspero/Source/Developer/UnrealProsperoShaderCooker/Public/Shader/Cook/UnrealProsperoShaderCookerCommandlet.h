#if 1 && !defined(UNREALPROSPERO_SHADER_COOKER_COMMANDLET_H)
#define UNREALPROSPERO_SHADER_COOKER_COMMANDLET_H

#include "Commandlets/Commandlet.h"
#include "UnrealProsperoShaderCookerCommandlet.generated.h"

UCLASS()
class UNREALPROSPEROSHADERCOOKER_API UUnrealProsperoShaderCookerCommandlet final : public UCommandlet
{
    GENERATED_BODY()

public:
    UUnrealProsperoShaderCookerCommandlet();
    int32 Main(const FString& parameters) override;
};

#endif
