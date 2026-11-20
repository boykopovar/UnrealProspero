#ifndef UNREALPROSPERO_SHADER_COOK_ARCHIVE_H
#define UNREALPROSPERO_SHADER_COOK_ARCHIVE_H

#include "ShaderCodeArchive.h"
#include "Memory/SharedBuffer.h"
#include <span>

namespace UnrealProspero::Shader::Cook
{
    class UNREALPROSPEROSHADERCOMPILER_API Archive final
    {
    public:
        explicit Archive(const FString& path);
        Archive(const Archive&) = delete;
        Archive& operator=(const Archive&) = delete;
        const FSerializedShaderArchive& GetIndex() const;
        TArray<uint8> GetCode(int32 index) const;
        void SetCode(int32 index, std::span<const std::uint8_t> bytes);
        void Save(const FString& path) const;

    private:
        FSerializedShaderArchive index;
        TArray<FSharedBuffer> code;
    };
}

#endif
