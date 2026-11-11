#include "UnrealProspero/RHI/Command/CommandList.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include <atomic>
#include <exception>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::RHI
{
    CommandList::CommandList(std::unique_ptr<ComputeContext> context) : context(std::move(context))
    {
        if (!this->context) throw std::invalid_argument("Command list requires a context");
        packet = this->context->finalize();
    }

    CommandList::~CommandList()
    {
        if (submitted && !IsComplete()) std::terminate();
    }

    void CommandList::Submit()
    {
        if (submitted) throw std::runtime_error("Command list has already been submitted");
        submitted = true;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        Kernel::CheckResult(sceAgcDriverSubmitDcb(&packet), "sceAgcDriverSubmitDcb");
    }

    bool CommandList::IsComplete() const { return submitted && context->completion.IsComplete(); }

    void CommandList::Wait(std::uint64_t timeoutMicroseconds) const
    {
        if (!submitted) throw std::runtime_error("Command list has not been submitted");
        context->completion.Wait(timeoutMicroseconds);
    }
}
