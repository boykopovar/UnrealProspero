#ifndef UNREALPROSPERO_RHI_UNIFORM_BUFFER_BINDING_H
#define UNREALPROSPERO_RHI_UNIFORM_BUFFER_BINDING_H

#include "UnrealProspero/RHI/Buffer/UniformBuffer.h"
#include "UnrealProspero/RHI/Descriptor/Tables.h"
#include "UnrealProspero/RHI/Shader/ShaderCode.h"

namespace UnrealProspero::RHI
{
    struct BoundUniformResource
    {
        FRHIShaderParameterResource::EType Type;
        std::uint32_t ParameterIndex;
        TRefCountPtr<FRHIResource> Resource;
    };

    class UNREALPROSPERORHI_API UniformBufferBinding final
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
