#include "AnyUnreal/Shader/ShaderFormat.h"
#include "Interfaces/IShaderFormatModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include <memory>

class FAnyUnrealShaderFormatModule final : public IShaderFormatModule
{
public:
    IShaderFormat* GetShaderFormat() override
    {
        if (!format) format = std::make_unique<AnyUnreal::Shader::ShaderFormat>(UTF8_TO_TCHAR(ANYUNREAL_ADK_EXECUTABLE), FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("AnyUnreal/ShaderCompiler"))));
        return format.get();
    }

    void ShutdownModule() override
    {
        format.reset();
    }

private:
    std::unique_ptr<AnyUnreal::Shader::ShaderFormat> format;
};

IMPLEMENT_MODULE(FAnyUnrealShaderFormatModule, AnyUnrealShaderFormat)
