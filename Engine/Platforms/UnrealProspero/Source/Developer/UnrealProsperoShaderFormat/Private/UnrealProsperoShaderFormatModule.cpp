#include "UnrealProspero/Shader/ShaderFormat.h"
#include "Interfaces/IShaderFormatModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include <memory>

class FUnrealProsperoShaderFormatModule final : public IShaderFormatModule
{
public:
    IShaderFormat* GetShaderFormat() override
    {
        if (!format) format = std::make_unique<UnrealProspero::Shader::ShaderFormat>(UTF8_TO_TCHAR(UNREALPROSPERO_ADK_EXECUTABLE), FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("UnrealProspero/ShaderCompiler"))));
        return format.get();
    }

    void ShutdownModule() override
    {
        format.reset();
    }

private:
    std::unique_ptr<UnrealProspero::Shader::ShaderFormat> format;
};

IMPLEMENT_MODULE(FUnrealProsperoShaderFormatModule, UnrealProsperoShaderFormat)
