#ifndef UNREALPROSPERO_RHI_COMMAND_LIST_H
#define UNREALPROSPERO_RHI_COMMAND_LIST_H

#include "UnrealProspero/RHI/Context/ComputeContext.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API CommandList final : public IRHIPlatformCommandList
    {
    public:
        explicit CommandList(std::unique_ptr<ComputeContext> context);
        ~CommandList();
        void Submit();
        bool IsComplete() const;
        std::shared_ptr<const Completion> CaptureCompletion() const;
        void Wait(std::uint64_t timeoutMicroseconds) const;

    private:
        std::unique_ptr<ComputeContext> context;
        Packet packet;
        bool submitted = false;
    };
}

#endif
