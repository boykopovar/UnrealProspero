#ifndef ANYUNREAL_SHADER_USER_DATA_H
#define ANYUNREAL_SHADER_USER_DATA_H

#include "AnyUnreal/Shader/ShaderBinary.h"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyUnreal::Shader
{
    struct UserDataInputs
    {
        std::array<std::uint64_t, 4> DescriptorSets{};
        std::uint64_t VertexBuffers = 0;
        std::uint64_t PushConstants = 0;
        std::uint64_t RingTable = 0;
        std::uint32_t BaseVertex = 0;
        std::uint32_t StartInstance = 0;
        std::uint32_t DrawId = 0;
        std::uint32_t ViewIndex = 0;
    };

    class ANYUNREALSHADERCOMMON_API UserDataLayout final
    {
    public:
        static UserDataLayout Read(std::span<const std::uint8_t> metadata, ShaderStage stage);
        std::array<std::uint32_t, 16> Build(const UserDataInputs& inputs) const;
        std::uint32_t GetCount() const;
        std::uint32_t GetAddressHigh() const;
        std::uint32_t GetPushConstantSize() const;
        ShaderStage GetStage() const;
        std::uint32_t GetVertexBufferUsageMask() const;
        bool UsesVertexBuffersPerAttribute() const;

    private:
        enum class Input { DescriptorSet, VertexBuffers, PushConstants, RingTable, BaseVertex, StartInstance, DrawId, ViewIndex, Constant };
        struct Binding { Input Kind; std::uint32_t Slot; std::uint32_t Value; };
        UserDataLayout() = default;
        std::vector<Binding> bindings;
        std::uint32_t count = 0;
        std::uint32_t addressHigh = 0;
        std::uint32_t pushConstantSize = 0;
        std::uint32_t vertexBufferUsageMask = 0;
        bool vertexBuffersPerAttribute = false;
        ShaderStage stage = ShaderStage::Vertex;
    };
}

#endif
