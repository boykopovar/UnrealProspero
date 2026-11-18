#ifndef UNREALPROSPERO_SHADER_FORMAT_H
#define UNREALPROSPERO_SHADER_FORMAT_H

#include "ShaderCompilerCommon.h"
#include "Misc/SecureHash.h"

namespace UnrealProspero::Shader
{
    class UNREALPROSPEROSHADERFORMAT_API ShaderFormat final : public UE::ShaderCompilerCommon::FBaseShaderFormat, public IShaderPipelineCompiler
    {
    public:
        ShaderFormat(const FString& executable, const FString& workingRoot);
        const IShaderPipelineCompiler* GetPipelineCompiler() const override;
        void CompilePreprocessedShaderPipeline(TConstArrayView<FShaderPipelineStage> stages) const override;
        uint32 GetVersion(FName format) const override;
        void GetSupportedFormats(TArray<FName>& formats) const override;
        const TCHAR* GetPlatformIncludeDirectory() const override;
        void ModifyShaderCompilerInput(FShaderCompilerInput& input) const override;
        void AppendToKeyString(FString& key) const override;
        void CompilePreprocessedShader(const FShaderCompilerInput& input, const FShaderPreprocessOutput& preprocessed, FShaderCompilerOutput& output) const override;

    private:
        FString executable;
        FString workingRoot;
        FSHAHash compilerHash;
        uint32 version;
    };
}

#endif
