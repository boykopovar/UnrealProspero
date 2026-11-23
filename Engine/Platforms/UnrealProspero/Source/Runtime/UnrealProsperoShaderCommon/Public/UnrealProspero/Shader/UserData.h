#ifndef UNREALPROSPERO_SHADER_USER_DATA_H
#define UNREALPROSPERO_SHADER_USER_DATA_H

#include "UnrealProspero/Shader/ShaderBinary.h"
#include <array>
#include <cstdint>
#include <span>
#include <optional>
#include <vector>

namespace UnrealProspero::Shader
{
    struct UserDataInputs
    {
        std::array<std::uint64_t, 4> DescriptorSets{};
        std::uint64_t VertexBuffers = 0;
        std::uint64_t PushConstants = 0;
        std::uint64_t RingTable = 0;
        std::uint64_t ScratchAddress = 0;
        std::uint32_t BaseVertex = 0;
        std::uint32_t StartInstance = 0;
        std::uint32_t DrawId = 0;
        std::uint32_t ViewIndex = 0;
    };

    class UNREALPROSPEROSHADERCOMMON_API UserDataLayout final
    {
    public:
        static UserDataLayout Read(std::span<const std::uint8_t> metadata, ShaderStage stage);
        std::array<std::uint32_t, 16> Build(const UserDataInputs& inputs) const;
        bool operator==(const UserDataLayout&) const = default;
        std::uint32_t GetCount() const;
        std::uint32_t GetAddressHigh() const;
        std::uint32_t GetPushConstantSize() const;
        std::uint32_t GetScratchBytesPerWave() const;
        ShaderStage GetStage() const;
        std::uint32_t GetVertexBufferUsageMask() const;
        bool UsesVertexBuffersPerAttribute() const;
        bool UsesDynamicVertexInput() const;
        std::optional<std::uint32_t> GetBaseVertexSlot() const;
        std::optional<std::uint32_t> GetStartInstanceSlot() const;

    private:
        enum class Input { DescriptorSet, VertexBuffers, PushConstants, RingTable, ScratchAddress, BaseVertex, StartInstance, DrawId, ViewIndex, Constant };
        struct Binding { Input Kind; std::uint32_t Slot; std::uint32_t Value; bool operator==(const Binding&) const = default; };
        UserDataLayout() = default;
        std::vector<Binding> bindings;
        std::uint32_t count = 0;
        std::uint32_t addressHigh = 0;
        std::uint32_t pushConstantSize = 0;
        std::uint32_t scratchBytesPerWave = 0;
        std::uint32_t vertexBufferUsageMask = 0;
        bool vertexBuffersPerAttribute = false;
        bool dynamicVertexInput = false;
        ShaderStage stage = ShaderStage::Vertex;
    };
}

#endif
