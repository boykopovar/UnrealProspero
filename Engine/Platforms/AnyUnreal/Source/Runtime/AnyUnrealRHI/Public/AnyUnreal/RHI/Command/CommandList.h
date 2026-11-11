#ifndef ANYUNREAL_RHI_COMMAND_LIST_H
#define ANYUNREAL_RHI_COMMAND_LIST_H

#include "AnyUnreal/RHI/Context/ComputeContext.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API CommandList final : public IRHIPlatformCommandList
    {
    public:
        explicit CommandList(std::unique_ptr<ComputeContext> context);
        ~CommandList();
        void Submit();
        bool IsComplete() const;
        void Wait(std::uint64_t timeoutMicroseconds) const;

    private:
        std::unique_ptr<ComputeContext> context;
        Packet packet;
        bool submitted = false;
    };
}

#endif
