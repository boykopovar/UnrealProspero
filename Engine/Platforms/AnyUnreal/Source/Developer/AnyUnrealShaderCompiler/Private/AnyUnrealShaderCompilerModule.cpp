#include "Modules/ModuleManager.h"
#include "DXCWrapper.h"
#include "ShaderConductorContext.h"

class FAnyUnrealShaderCompilerModule final : public IModuleInterface, public FShaderConductorModuleWrapper
{
public:
    void ShutdownModule() override
    {
        CrossCompiler::FShaderConductorContext::Shutdown();
    }
};

IMPLEMENT_MODULE(FAnyUnrealShaderCompilerModule, AnyUnrealShaderCompiler)
