#include "AnyUnreal/Video/Output.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceVideoOut/include/Output.hpp"
#include <stdexcept>

namespace AnyUnreal::Video
{
    Output::Output(int userId)
    {
        const int opened = sceVideoOutOpen(userId, 0, 0, nullptr);
        if (opened < 0) throw Kernel::Error(opened, "sceVideoOutOpen");
        handle = opened;
    }

    Output::~Output() noexcept(false)
    {
        Close();
    }

    int Output::GetHandle() const
    {
        if (!handle) throw std::logic_error("Video output is closed");
        return *handle;
    }

    VideoOutResolutionStatus Output::GetResolution() const
    {
        VideoOutResolutionStatus status{};
        Kernel::CheckResult(sceVideoOutGetResolutionStatus(GetHandle(), &status), "sceVideoOutGetResolutionStatus");
        if (status.fullWidth == 0 || status.fullHeight == 0 || status.paneWidth == 0 || status.paneHeight == 0) throw std::runtime_error("Video output reported an empty resolution");
        return status;
    }

    void Output::SetFlipRate(int rate)
    {
        if (rate < 0 || rate > 2) throw std::invalid_argument("Invalid video flip rate");
        Kernel::CheckResult(sceVideoOutSetFlipRate(GetHandle(), rate), "sceVideoOutSetFlipRate");
    }

    void Output::WaitVblank()
    {
        Kernel::CheckResult(sceVideoOutWaitVblank(GetHandle()), "sceVideoOutWaitVblank");
    }

    void Output::Close()
    {
        if (!handle) return;
        Kernel::CheckResult(sceVideoOutClose(*handle), "sceVideoOutClose");
        handle.reset();
    }
}
