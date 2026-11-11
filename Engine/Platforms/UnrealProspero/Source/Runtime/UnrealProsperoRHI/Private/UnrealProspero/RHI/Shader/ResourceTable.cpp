#include "UnrealProspero/RHI/Shader/ResourceTable.h"
#include <array>
#include <bit>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    std::vector<ResourceTableBinding> ReadResourceTableBindings(const FShaderResourceTable& table)
    {
        using Type = FRHIShaderParameterResource::EType;
        struct Stream
        {
            const TArray<uint32>* Tokens;
            Type ResourceType;
        };
        const std::array streams{
            Stream{&table.TextureMap, Type::Texture},
            Stream{&table.ShaderResourceViewMap, Type::ResourceView},
            Stream{&table.SamplerMap, Type::Sampler},
            Stream{&table.UnorderedAccessViewMap, Type::UnorderedAccessView},
            Stream{&table.ResourceCollectionMap, Type::ResourceCollection}
        };
        const auto prefixSize = std::bit_width(table.ResourceTableBits);
        if (table.ResourceTableLayoutHashes.Num() > 32 || static_cast<unsigned>(table.ResourceTableLayoutHashes.Num()) < prefixSize) throw std::invalid_argument("Resource table layout hashes do not cover its uniform buffers");
        std::vector<ResourceTableBinding> result;
        std::uint32_t usedBuffers = 0;
        for (const auto& stream : streams)
        {
            const auto& tokens = *stream.Tokens;
            if (tokens.IsEmpty()) continue;
            if (static_cast<unsigned>(tokens.Num()) <= prefixSize || tokens.Last() != FRHIResourceTableEntry::GetEndOfStreamToken()) throw std::invalid_argument("Invalid resource table token stream");
            std::array<std::uint32_t, 32> offsets{};
            std::array<bool, 256> parameters{};
            for (unsigned index = prefixSize; index + 1 < static_cast<unsigned>(tokens.Num()); ++index)
            {
                const auto token = tokens[index];
                const auto uniform = FRHIResourceTableEntry::GetUniformBufferIndex(token);
                const auto parameter = FRHIResourceTableEntry::GetBindIndex(token);
                if (uniform >= prefixSize || (table.ResourceTableBits & (std::uint32_t{1} << uniform)) == 0 || (index > prefixSize && tokens[index - 1] >= token)) throw std::invalid_argument("Invalid or unsorted resource table entry");
                if (parameters[parameter]) throw std::invalid_argument("Resource table parameter is bound more than once");
                parameters[parameter] = true;
                if (offsets[uniform] == 0) offsets[uniform] = index;
                usedBuffers |= std::uint32_t{1} << uniform;
                result.push_back({stream.ResourceType, uniform, FRHIResourceTableEntry::GetResourceIndex(token), parameter});
            }
            for (unsigned index = 0; index < prefixSize; ++index)
            {
                if (tokens[index] != offsets[index]) throw std::invalid_argument("Resource table offset does not match its token stream");
            }
        }
        if (usedBuffers != table.ResourceTableBits) throw std::invalid_argument("Resource table mask does not match its resources");
        return result;
    }
}
