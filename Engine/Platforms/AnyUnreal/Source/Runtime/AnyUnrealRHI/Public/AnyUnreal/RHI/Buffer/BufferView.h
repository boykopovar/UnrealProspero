#ifndef ANYUNREAL_RHI_BUFFER_VIEW_H
#define ANYUNREAL_RHI_BUFFER_VIEW_H

#include "AnyUnreal/RHI/Buffer/Buffer.h"
#include <array>
#include <type_traits>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API const FRHIViewDesc& CheckBufferViewDescription(const Buffer& buffer, const FRHIViewDesc& description, FRHIViewDesc::EViewType expectedType);

    class ANYUNREALRHI_API BufferViewData final
    {
    public:
        BufferViewData(Buffer& buffer, const FRHIViewDesc& description);
        const std::array<std::uint32_t, 4>& GetDescriptor() const;
        const BufferAllocation* GetStorage() const { return storage.get(); }
        std::span<std::byte> GetRange() const { return range; }

    private:
        std::shared_ptr<BufferAllocation> storage;
        std::array<std::uint32_t, 4> descriptor{};
        std::span<std::byte> range;
    };

    template<typename TBase, FRHIViewDesc::EViewType TType>
    class BufferView final : public TBase
    {
        static_assert((std::is_same_v<TBase, FRHIShaderResourceView> && TType == FRHIViewDesc::EViewType::BufferSRV) || (std::is_same_v<TBase, FRHIUnorderedAccessView> && TType == FRHIViewDesc::EViewType::BufferUAV));

    public:
        BufferView(Buffer& buffer, const FRHIViewDesc& description) : TBase(&buffer, CheckBufferViewDescription(buffer, description, TType)), data(buffer, description) {}
        const std::array<std::uint32_t, 4>& GetDescriptor() const { return data.GetDescriptor(); }
        const BufferAllocation* GetStorage() const { return data.GetStorage(); }
        std::span<std::byte> GetRange() const { return data.GetRange(); }

    private:
        BufferViewData data;
    };

    extern template class ANYUNREALRHI_API BufferView<FRHIShaderResourceView, FRHIViewDesc::EViewType::BufferSRV>;
    extern template class ANYUNREALRHI_API BufferView<FRHIUnorderedAccessView, FRHIViewDesc::EViewType::BufferUAV>;
    using BufferShaderResourceView = BufferView<FRHIShaderResourceView, FRHIViewDesc::EViewType::BufferSRV>;
    using BufferUnorderedAccessView = BufferView<FRHIUnorderedAccessView, FRHIViewDesc::EViewType::BufferUAV>;
}

#endif
