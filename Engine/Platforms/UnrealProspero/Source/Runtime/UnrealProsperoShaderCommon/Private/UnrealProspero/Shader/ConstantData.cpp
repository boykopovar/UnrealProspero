#include "UnrealProspero/Shader/ConstantData.h"
#include <cstring>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    ConstantData::ConstantData(std::span<const RuntimeBinding> bindings)
    {
        ValidateRuntimeBindings(bindings);
        for (const auto& binding : bindings)
        {
            if (binding.Kind != ResourceKind::UniformBuffer) continue;
            if (binding.Count != 1) throw std::runtime_error("Uniform buffer descriptor arrays are not implemented");
            blocks.emplace(binding.ParameterIndex, ConstantBlock{binding.BlockSize, {}});
        }
    }

    void ConstantData::Write(std::uint32_t index, std::size_t destinationOffset, std::span<const std::byte> source, std::size_t sourceOffset, std::size_t length)
    {
        if (length == 0 || sourceOffset > source.size() || length > source.size() - sourceOffset) throw std::out_of_range("Shader constant source range is invalid");
        const auto position = blocks.find(index);
        if (position == blocks.end()) throw std::invalid_argument("Shader constant buffer binding does not exist");
        auto& block = position->second;
        if (destinationOffset > block.Size || length > block.Size - destinationOffset) throw std::out_of_range("Shader constant destination range is invalid");
        if (block.Data.empty()) block.Data.resize(block.Size);
        std::memmove(block.Data.data() + destinationOffset, source.data() + sourceOffset, length);
    }

    bool ConstantData::Contains(std::uint32_t index) const
    {
        const auto position = blocks.find(index);
        return position != blocks.end() && !position->second.Data.empty();
    }

    const std::map<std::uint32_t, ConstantBlock>& ConstantData::GetBlocks() const { return blocks; }
}
