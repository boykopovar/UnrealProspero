#include "UnrealProspero/RHI/Profiling/FrameProfiler.h"
#include "GPUProfiler.h"
#include "HAL/PlatformTime.h"
#include <deque>
#include <exception>
#include <utility>

namespace UnrealProspero::RHI
{
    namespace
    {
        constexpr UE::RHI::GPUProfiler::FQueue graphicsQueue{UE::RHI::GPUProfiler::FQueue::EType::Graphics, 0, 0};

        struct Frame
        {
            std::shared_ptr<const Completion> Marker;
            UE::RHI::GPUProfiler::FEventStream Events{graphicsQueue};

            Frame(const FRHIEndFrameArgs& arguments, std::shared_ptr<const Completion> completion) : Marker(std::move(completion))
            {
                Events.Emplace<UE::RHI::GPUProfiler::FEvent::FFrameBoundary>(ERHIPipeline::Graphics, arguments, FPlatformTime::Cycles64());
            }
        };
    }

    struct FrameProfiler::State
    {
        std::deque<Frame> Frames;
    };

    FrameProfiler::FrameProfiler() : state(std::make_unique<State>())
    {
        UE::RHI::GPUProfiler::InitializeQueues(MakeConstArrayView(&graphicsQueue, 1));
    }

    FrameProfiler::~FrameProfiler()
    {
        if (!state->Frames.empty()) std::terminate();
    }

    void FrameProfiler::EndFrame(const FRHIEndFrameArgs& arguments, std::shared_ptr<const Completion> completion)
    {
        state->Frames.emplace_back(arguments, std::move(completion));
    }

    void FrameProfiler::Poll()
    {
        while (!state->Frames.empty())
        {
            auto& frame = state->Frames.front();
            if (frame.Marker && !frame.Marker->IsComplete()) return;
            UE::RHI::GPUProfiler::ProcessEvents(MakeArrayView(&frame.Events, 1));
            state->Frames.pop_front();
        }
    }
}
