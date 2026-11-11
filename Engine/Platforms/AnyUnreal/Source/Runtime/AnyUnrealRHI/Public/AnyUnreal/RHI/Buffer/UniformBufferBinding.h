#ifndef ANYUNREAL_RHI_UNIFORM_BUFFER_BINDING_H
#define ANYUNREAL_RHI_UNIFORM_BUFFER_BINDING_H

#include "AnyUnreal/RHI/Buffer/UniformBuffer.h"
#include "AnyUnreal/RHI/Descriptor/Tables.h"
#include "AnyUnreal/RHI/Shader/ShaderCode.h"

namespace AnyUnreal::RHI
{
    struct BoundUniformResource
    {
        FRHIShaderParameterResource::EType Type;
        std::uint32_t ParameterIndex;
        TRefCountPtr<FRHIResource> Resource;
    };

    class ANYUNREALRHI_API UniformBufferBinding final
    {
    public:
        UniformBufferBinding(const ShaderCode& shader, std::uint32_t parameterIndex, const UniformBuffer& buffer);
        void ApplyConstants(DescriptorTables& descriptors) const;
        const BufferAllocation* GetConstants() const;
        const std::vector<BoundUniformResource>& GetResources() const;

    private:
        std::uint32_t parameterIndex;
        bool hasConstants = false;
        std::shared_ptr<const UniformBufferStorage> storage;
        std::vector<BoundUniformResource> resources;
    };
}

#endif
