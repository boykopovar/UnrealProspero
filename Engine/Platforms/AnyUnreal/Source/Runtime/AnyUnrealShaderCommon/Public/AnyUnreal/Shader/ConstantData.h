#ifndef ANYUNREAL_SHADER_CONSTANT_DATA_H
#define ANYUNREAL_SHADER_CONSTANT_DATA_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include <cstddef>
#include <map>

namespace AnyUnreal::Shader
{
    struct ConstantBlock
    {
        std::uint32_t Size;
        std::vector<std::byte> Data;
    };

    class ANYUNREALSHADERCOMMON_API ConstantData final
    {
    public:
        explicit ConstantData(std::span<const RuntimeBinding> bindings);
        void Write(std::uint32_t index, std::size_t destinationOffset, std::span<const std::byte> source, std::size_t sourceOffset, std::size_t length);
        bool Contains(std::uint32_t index) const;
        const std::map<std::uint32_t, ConstantBlock>& GetBlocks() const;

    private:
        std::map<std::uint32_t, ConstantBlock> blocks;
    };
}

#endif
