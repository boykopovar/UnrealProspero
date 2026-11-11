#include "UnrealProspero/RHI/Buffer/UniformBufferStorage.h"
#include "ShaderParameterStruct.h"
#include <cstring>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    UniformBufferStorage::UniformBufferStorage(const FRHIUniformBufferLayout& layout, const void* contents, EUniformBufferValidation validation, std::uint32_t addressHigh)
    {
        if (validation != EUniformBufferValidation::None && validation != EUniformBufferValidation::ValidateResources) throw std::invalid_argument("Invalid uniform buffer validation mode");
        if (layout.ConstantBufferSize == 0 && layout.Resources.IsEmpty()) throw std::invalid_argument("Uniform buffer layout is empty");
        if (layout.ConstantBufferSize % 4 != 0) throw std::invalid_argument("Uniform buffer constant size is not word aligned");
        if (EnumHasAnyFlags(layout.Flags, ERHIUniformBufferFlags::UniformView | ERHIUniformBufferFlags::BindlessAccessible)) throw std::runtime_error("Uniform buffer views and bindless buffers are not implemented");
        if (contents && reinterpret_cast<std::uintptr_t>(contents) % alignof(void*) != 0 && !layout.Resources.IsEmpty()) throw std::invalid_argument("Uniform buffer resource pointers are unaligned");
        resources.Reserve(layout.Resources.Num());
        for (const auto& member : layout.Resources)
        {
            if (member.MemberOffset % alignof(void*) != 0) throw std::invalid_argument("Uniform buffer resource offset is unaligned");
            FRHIResource* resource = contents ? GetShaderParameterResourceRHI(contents, member.MemberOffset, member.MemberType) : nullptr;
            if (contents && validation == EUniformBufferValidation::ValidateResources && !resource && !IsShaderParameterTypeIgnoredByRHI(member.MemberType)) throw std::invalid_argument("Uniform buffer resource is null");
            resources.Emplace(resource);
        }
        if (layout.ConstantBufferSize != 0)
        {
            constants = std::make_unique<BufferAllocation>(layout.ConstantBufferSize, addressHigh);
            if (contents) std::memcpy(constants->Data(), contents, layout.ConstantBufferSize);
        }
    }

    const BufferAllocation* UniformBufferStorage::GetConstants() const { return constants.get(); }
    const TArray<TRefCountPtr<FRHIResource>>& UniformBufferStorage::GetResources() const { return resources; }
}
