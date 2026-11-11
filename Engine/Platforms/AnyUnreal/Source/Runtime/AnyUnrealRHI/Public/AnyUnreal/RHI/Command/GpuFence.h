#ifndef ANYUNREAL_RHI_GPU_FENCE_H
#define ANYUNREAL_RHI_GPU_FENCE_H

#include "AnyUnreal/RHI/Command/Completion.h"
#include "HAL/CriticalSection.h"
#include "RHIResources.h"
#include <memory>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API GpuFence final : public FRHIGPUFence
    {
    public:
        explicit GpuFence(FName name);
        void Clear() override;
        bool Poll() const override;
        bool Poll(FRHIGPUMask gpuMask) const override;
        void Wait(FRHICommandListImmediate& commandList, FRHIGPUMask gpuMask) const override;
        void Write(CommandBuffer& commands, const std::shared_ptr<Completion>& marker);

    private:
        mutable FCriticalSection mutex;
        std::shared_ptr<Completion> completion;
    };
}

#endif
