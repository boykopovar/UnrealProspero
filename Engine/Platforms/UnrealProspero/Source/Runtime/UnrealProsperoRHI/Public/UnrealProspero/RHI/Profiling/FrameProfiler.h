#ifndef UNREALPROSPERO_RHI_FRAME_PROFILER_H
#define UNREALPROSPERO_RHI_FRAME_PROFILER_H

#include "UnrealProspero/RHI/Command/Completion.h"
#include <memory>

struct FRHIEndFrameArgs;

namespace UnrealProspero::RHI
{
    class FrameProfiler final
    {
    public:
        FrameProfiler();
        ~FrameProfiler();
        void EndFrame(const FRHIEndFrameArgs& arguments, std::shared_ptr<const Completion> completion);
        void Poll();

    private:
        struct State;
        std::unique_ptr<State> state;
    };
}

#endif
