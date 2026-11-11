#include "UnrealProspero/RHI/Command/GpuFence.h"
#include "Misc/ScopeLock.h"
#include "RHICommandList.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    GpuFence::GpuFence(FName name) : FRHIGPUFence(name) {}

    void GpuFence::Clear()
    {
        FScopeLock lock(&mutex);
        completion.reset();
    }

    bool GpuFence::Poll() const
    {
        FScopeLock lock(&mutex);
        return NumPendingWriteCommands.GetValue() == 0 && completion && completion->IsComplete();
    }

    bool GpuFence::Poll(FRHIGPUMask gpuMask) const
    {
        if (gpuMask != FRHIGPUMask::GPU0()) throw std::invalid_argument("GPU fence supports only GPU zero");
        return Poll();
    }

    void GpuFence::Wait(FRHICommandListImmediate& commandList, FRHIGPUMask gpuMask) const
    {
        if (gpuMask != FRHIGPUMask::GPU0()) throw std::invalid_argument("GPU fence supports only GPU zero");
        commandList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
        std::shared_ptr<Completion> marker;
        {
            FScopeLock lock(&mutex);
            if (NumPendingWriteCommands.GetValue() != 0 || !completion) throw std::runtime_error("GPU fence has no submitted completion marker");
            marker = completion;
        }
        marker->Wait(10000000);
    }

    void GpuFence::Write(CommandBuffer& commands, const std::shared_ptr<Completion>& marker)
    {
        if (!marker) throw std::invalid_argument("GPU fence requires a completion marker");
        FScopeLock lock(&mutex);
        marker->Write(commands);
        completion = marker;
    }
}
