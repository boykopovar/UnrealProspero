#include "AnyUnreal/RHI/Buffer/BufferInitializer.h"
#include "AnyUnreal/RHI/Buffer/Buffer.h"

namespace AnyUnreal::RHI
{
    class BufferInitializer final : public FRHIBufferInitializer
    {
    public:
        BufferInitializer(FRHICommandListBase& commandList, TRefCountPtr<AnyUnreal::RHI::Buffer> buffer, bool initialize) : FRHIBufferInitializer(commandList, buffer.GetReference(), initialize ? buffer->beginInitialization() : nullptr, initialize ? buffer->GetSize() : 0, [buffer, initialize](FRHICommandListBase&) -> FBufferRHIRef
        {
            if (initialize) buffer->finishInitialization();
            return FBufferRHIRef(buffer.GetReference());
        }) {}
    };

    FRHIBufferInitializer CreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description, std::uint32_t addressHigh)
    {
        TRefCountPtr<AnyUnreal::RHI::Buffer> buffer = new AnyUnreal::RHI::Buffer(description, addressHigh);
        return BufferInitializer(commandList, buffer, description.InitAction == ERHIBufferInitAction::Initializer);
    }
}
