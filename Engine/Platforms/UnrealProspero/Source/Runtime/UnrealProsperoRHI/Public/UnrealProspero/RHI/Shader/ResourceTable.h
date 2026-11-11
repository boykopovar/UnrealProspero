#ifndef UNREALPROSPERO_RHI_SHADER_RESOURCE_TABLE_H
#define UNREALPROSPERO_RHI_SHADER_RESOURCE_TABLE_H

#include "RHIResources.h"
#include "RHIShaderParameters.h"
#include <vector>

namespace UnrealProspero::RHI
{
    struct ResourceTableBinding
    {
        FRHIShaderParameterResource::EType Type;
        std::uint32_t UniformBufferIndex;
        std::uint32_t ResourceIndex;
        std::uint32_t ParameterIndex;
    };

    UNREALPROSPERORHI_API std::vector<ResourceTableBinding> ReadResourceTableBindings(const FShaderResourceTable& table);
}

#endif
