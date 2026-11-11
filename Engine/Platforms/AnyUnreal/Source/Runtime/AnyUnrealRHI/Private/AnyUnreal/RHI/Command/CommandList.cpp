#include "AnyUnreal/RHI/Command/CommandList.h"
#include <stdexcept>
#include <utility>

namespace AnyUnreal::RHI
{
    CommandList::CommandList(std::unique_ptr<ComputeContext> context) : context(std::move(context))
    {
        if (!this->context) throw std::invalid_argument("Command list requires a context");
        packet = this->context->finalize();
    }

    const Packet& CommandList::GetPacket() const { return packet; }
}
