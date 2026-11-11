#include "Modules/ModuleManager.h"
#include "DXCWrapper.h"
#include "ShaderConductorContext.h"
#include "UnrealProspero/Shader/CompilerVersion.h"

class FUnrealProsperoShaderCompilerModule final : public IModuleInterface, public FShaderConductorModuleWrapper
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

std::uint32_t UnrealProspero::Shader::GetShaderConductorVersion()
{
    return FModuleManager::LoadModuleChecked<FUnrealProsperoShaderCompilerModule>(TEXT("UnrealProsperoShaderCompiler")).GetVersion();
}

IMPLEMENT_MODULE(FUnrealProsperoShaderCompilerModule, UnrealProsperoShaderCompiler)
