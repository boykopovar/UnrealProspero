#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include "Containers/ResourceArray.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        const FRHIBufferCreateDesc& checkedDescription(const FRHIBufferCreateDesc& description)
        {
            using Usage = EBufferUsageFlags;
            if (description.GPUMask.GetNative() != 1) throw std::runtime_error("Multiple GPU buffers are not implemented");
            if (EnumHasAnyFlags(description.Usage, Usage::NullResource))
            {
                if (description.Usage != Usage::NullResource || description.Size != 0 || description.Stride != 0 || description.InitialData || description.InitAction != ERHIBufferInitAction::Default) throw std::invalid_argument("Invalid null buffer description");
                return description;
            }
            constexpr auto supported = Usage::Static | Usage::Dynamic | Usage::Volatile | Usage::UnorderedAccess | Usage::ByteAddressBuffer | Usage::SourceCopy | Usage::DrawIndirect | Usage::ShaderResource | Usage::KeepCPUAccessible | Usage::FastVRAM | Usage::NNE | Usage::VertexBuffer | Usage::IndexBuffer | Usage::StructuredBuffer | Usage::MultiGPUGraphIgnore | Usage::UniformBuffer;
            if (EnumHasAnyFlags(description.Usage, ~supported)) throw std::runtime_error("Buffer usage requires an unimplemented allocation mode");
            if (description.Size == 0) throw std::invalid_argument("Buffer size is zero");
            if (EnumHasAnyFlags(description.Usage, Usage::IndexBuffer) && description.Stride != 2 && description.Stride != 4) throw std::invalid_argument("Invalid index buffer stride");
            if (EnumHasAnyFlags(description.Usage, Usage::StructuredBuffer) && description.Stride == 0) throw std::invalid_argument("Structured buffer stride is zero");
            if (description.Stride != 0 && description.Size % description.Stride != 0) throw std::invalid_argument("Buffer size is not a multiple of its stride");
            if (EnumHasAnyFlags(description.Usage, Usage::ByteAddressBuffer) && description.Size % 4 != 0) throw std::invalid_argument("Byte-address buffer size is not word aligned");
            switch (description.InitAction)
            {
            case ERHIBufferInitAction::Default:
            case ERHIBufferInitAction::Zeroed:
                if (description.InitialData) throw std::invalid_argument("Buffer data requires the resource-array initialization action");
                break;
            case ERHIBufferInitAction::ResourceArray:
                if (!description.InitialData || !description.InitialData->GetResourceData() || description.InitialData->GetResourceDataSize() != description.Size) throw std::invalid_argument("Buffer initial data does not match its size");
                break;
            case ERHIBufferInitAction::Initializer:
                if (description.InitialData) throw std::invalid_argument("Deferred buffer initialization cannot also upload a resource array");
                break;
            default:
                throw std::invalid_argument("Unknown buffer initialization action");
            }
            return description;
        }
    }

    Buffer::Buffer(const FRHIBufferCreateDesc& description, std::uint32_t addressHigh) : FRHIBuffer(checkedDescription(description))
    {
        if (description.Usage == EBufferUsageFlags::NullResource) return;
        storage = std::make_shared<BufferAllocation>(description.Size, addressHigh);
        if (description.InitAction == ERHIBufferInitAction::Initializer) initializationState = InitializationState::Pending;
        if (description.InitAction == ERHIBufferInitAction::ResourceArray)
        {
            const auto* source = static_cast<const std::byte*>(description.InitialData->GetResourceData());
            storage->Upload(0, {source, description.Size});
            description.InitialData->Discard();
        }
    }

    void* Buffer::beginInitialization()
    {
        if (initializationState != InitializationState::Pending || !storage) throw std::runtime_error("Buffer is not awaiting initialization");
        initializationState = InitializationState::Writing;
        return storage->Data();
    }

    void Buffer::finishInitialization()
    {
        if (initializationState != InitializationState::Writing) throw std::runtime_error("Buffer initialization has not started");
        initializationState = InitializationState::Ready;
    }

    std::shared_ptr<BufferAllocation> Buffer::Capture() const
    {
        if (initializationState != InitializationState::Ready) throw std::runtime_error("Buffer initialization has not been finalized");
        if (!storage) throw std::runtime_error("Null buffer has no GPU allocation");
        return storage;
    }
}
