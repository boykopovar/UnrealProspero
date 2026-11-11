#include "Modules/ModuleManager.h"
#include "DXCWrapper.h"
#include "ShaderConductorContext.h"
#include "AnyUnreal/Shader/CompilerVersion.h"

class FAnyUnrealShaderCompilerModule final : public IModuleInterface, public FShaderConductorModuleWrapper
{
public:
    std::uint32_t GetVersion() const
    {
        return GetModuleVersionHash();
    }

    void ShutdownModule() override
    {
        CrossCompiler::FShaderConductorContext::Shutdown();
    }
};

std::uint32_t AnyUnreal::Shader::GetShaderConductorVersion()
{
    return FModuleManager::LoadModuleChecked<FAnyUnrealShaderCompilerModule>(TEXT("AnyUnrealShaderCompiler")).GetVersion();
}

IMPLEMENT_MODULE(FAnyUnrealShaderCompilerModule, AnyUnrealShaderCompiler)
