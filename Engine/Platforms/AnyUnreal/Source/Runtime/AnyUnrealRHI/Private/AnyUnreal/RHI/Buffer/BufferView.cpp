#include "AnyUnreal/RHI/Buffer/BufferView.h"
#include "AnyUnreal/RHI/Descriptor/Buffer.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    template class BufferView<FRHIShaderResourceView, FRHIViewDesc::EViewType::BufferSRV>;
    template class BufferView<FRHIUnorderedAccessView, FRHIViewDesc::EViewType::BufferUAV>;

    const FRHIViewDesc& CheckBufferViewDescription(const Buffer& buffer, const FRHIViewDesc& description, FRHIViewDesc::EViewType expectedType)
    {
        using ViewType = FRHIViewDesc::EViewType;
        using BufferType = FRHIViewDesc::EBufferType;
        if ((expectedType != ViewType::BufferSRV && expectedType != ViewType::BufferUAV) || description.Common.ViewType != expectedType) throw std::invalid_argument("Buffer view type does not match the requested resource");
        const FRHIViewDesc::FBuffer& view = expectedType == ViewType::BufferSRV ? static_cast<const FRHIViewDesc::FBuffer&>(description.Buffer.SRV) : description.Buffer.UAV;
        if (view.BufferType != BufferType::Raw && view.BufferType != BufferType::Structured) throw std::runtime_error("Typed and acceleration structure buffer views are not implemented");
        if (view.Format != PF_Unknown) throw std::invalid_argument("Raw and structured buffer views cannot specify a typed format");
        if (view.bAtomicCounter || view.bAppendBuffer) throw std::runtime_error("Counter and append buffer views are not implemented");
        if (buffer.GetUsage() == EBufferUsageFlags::NullResource) return description;
        if (expectedType == ViewType::BufferUAV && !EnumHasAnyFlags(buffer.GetUsage(), EBufferUsageFlags::UnorderedAccess)) throw std::invalid_argument("Buffer does not allow unordered access");
        if (expectedType == ViewType::BufferSRV && !EnumHasAnyFlags(buffer.GetUsage(), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::ByteAddressBuffer)) throw std::invalid_argument("Buffer does not allow shader resource access");
        std::uint32_t stride = 4;
        if (view.BufferType == BufferType::Raw)
        {
            if (!EnumHasAnyFlags(buffer.GetUsage(), EBufferUsageFlags::ByteAddressBuffer) || view.Stride != 0 || view.OffsetInBytes % 16 != 0) throw std::invalid_argument("Invalid raw buffer view layout");
        }
        else
        {
            stride = view.Stride == 0 ? buffer.GetStride() : view.Stride;
            if (!EnumHasAnyFlags(buffer.GetUsage(), EBufferUsageFlags::StructuredBuffer) || stride == 0 || stride % 4 != 0 || view.OffsetInBytes % stride != 0) throw std::invalid_argument("Invalid structured buffer view layout");
        }
        if (view.OffsetInBytes >= buffer.GetSize() || (view.OffsetInBytes != 0 && view.NumElements == 0)) throw std::out_of_range("Invalid buffer view offset or element count");
        const auto count = view.NumElements == 0 ? buffer.GetSize() / stride : view.NumElements;
        const auto size = static_cast<std::uint64_t>(count) * stride;
        if (size == 0 || size > buffer.GetSize() - view.OffsetInBytes) throw std::out_of_range("Buffer view exceeds its allocation");
        return description;
    }

    BufferViewData::BufferViewData(Buffer& buffer, const FRHIViewDesc& description)
    {
        CheckBufferViewDescription(buffer, description, description.Common.ViewType);
        FRHIViewDesc::FBuffer::FViewInfo info;
        if (description.Common.ViewType == FRHIViewDesc::EViewType::BufferSRV) info = description.Buffer.SRV.GetViewInfo(&buffer);
        else info = description.Buffer.UAV.GetViewInfo(&buffer);
        if (info.bNullView) return;
        storage = buffer.Capture();
        range = storage->GetRange(info.OffsetInBytes, info.SizeInBytes);
        descriptor = CreateByteAddressBufferDescriptor(range.data(), range.size());
    }

    const std::array<std::uint32_t, 4>& BufferViewData::GetDescriptor() const { return descriptor; }
}
