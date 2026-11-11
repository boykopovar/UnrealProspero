#ifndef UNREALPROSPERO_RHI_STATIC_UNIFORM_BUFFERS_H
#define UNREALPROSPERO_RHI_STATIC_UNIFORM_BUFFERS_H

#include "Misc/Timespan.h"
#include "RHIContext.h"
#include <array>
#include <vector>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API StaticUniformBuffers final
    {
    public:
        void Set(FUniformBufferStaticSlot slot, FRHIUniformBuffer* buffer);
        void Replace(const FUniformBufferStaticBindings& bindings);
        std::vector<FRHIShaderParameterResource> Resolve(const FRHIShader& shader) const;

    private:
        std::array<TRefCountPtr<FRHIUniformBuffer>, MAX_UNIFORM_BUFFER_STATIC_SLOTS> buffers{};
    };
}

#endif
