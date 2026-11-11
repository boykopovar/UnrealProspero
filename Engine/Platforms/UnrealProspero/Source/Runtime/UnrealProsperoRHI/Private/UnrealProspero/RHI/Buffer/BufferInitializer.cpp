#include "UnrealProspero/RHI/Buffer/BufferInitializer.h"
#include "UnrealProspero/RHI/Buffer/Buffer.h"

namespace UnrealProspero::RHI
{
    class BufferInitializer final : public FRHIBufferInitializer
    {
    public:
        BufferInitializer(FRHICommandListBase& commandList, TRefCountPtr<UnrealProspero::RHI::Buffer> buffer, bool initialize) : FRHIBufferInitializer(commandList, buffer.GetReference(), initialize ? buffer->beginInitialization() : nullptr, initialize ? buffer->GetSize() : 0, [buffer, initialize](FRHICommandListBase&) -> FBufferRHIRef
        {
            if (initialize) buffer->finishInitialization();
            return FBufferRHIRef(buffer.GetReference());
        }) {}
    };

    FRHIBufferInitializer CreateBufferInitializer(FRHICommandListBase& commandList, const FRHIBufferCreateDesc& description, std::uint32_t addressHigh)
    {
        TRefCountPtr<UnrealProspero::RHI::Buffer> buffer = new UnrealProspero::RHI::Buffer(description, addressHigh);
        return BufferInitializer(commandList, buffer, description.InitAction == ERHIBufferInitAction::Initializer);
    }
}
