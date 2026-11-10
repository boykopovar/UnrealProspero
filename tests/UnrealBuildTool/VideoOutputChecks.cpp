#include "AnyUnreal/Video/Output.h"
#include "HAL/PlatformApplicationMisc.h"
#include "AnyUnreal/Kernel/Error.h"
#include "AnyUnreal/Kernel/DirectAllocation.h"
#include "AnyUnreal/Kernel/Clock.h"
#include "prx/libSceVideoOut/include/Buffer.hpp"
#include <array>
#include <stdexcept>

void WaitForFlip(AnyUnreal::Video::Output& output, std::int64_t argument)
{
    const auto start = AnyUnreal::Kernel::Clock::MonotonicNanoseconds();
    while (output.GetFlipStatus().flipArg != argument)
    {
        if (AnyUnreal::Kernel::Clock::MonotonicNanoseconds() - start > 5000000000ull) throw std::runtime_error("Video flip timed out");
        output.WaitVblank();
    }
}

int main()
{
    using AnyUnreal::Video::Output;
    for (int iteration = 0; iteration != 3; ++iteration)
    {
        AnyUnreal::Kernel::DirectAllocation pixels(262144, 65536, AnyUnreal::Kernel::DirectMemoryAccess::CpuGpu);
        Output output;
        static_cast<void>(FPlatformApplicationMisc::IsThisApplicationForeground());
        FPlatformApplicationMisc::PumpMessages(false);
        FPlatformApplicationMisc::PumpMessages(true);
        const auto resolution = output.GetResolution();
        if (resolution.fullWidth == 0 || resolution.fullHeight == 0) return 61;
        try
        {
            Output duplicate;
            return 62;
        }
        catch (const AnyUnreal::Kernel::Error&)
        {
        }
        const auto after = output.GetResolution();
        if (after.fullWidth != resolution.fullWidth || after.fullHeight != resolution.fullHeight) return 63;
        for (int rate = 0; rate != 3; ++rate) output.SetFlipRate(rate);
        try
        {
            output.SetFlipRate(3);
            return 64;
        }
        catch (const std::invalid_argument&)
        {
        }
        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 1, 256, 256, 0, 0, 0);
        attribute.pitch_in_pixel = 256;
        try
        {
            output.RegisterBuffers(1, 1, {}, attribute, 0);
            return 66;
        }
        catch (const std::invalid_argument&)
        {
        }
        const std::array buffers{VideoOutBuffers{pixels.Data(), nullptr, {nullptr, nullptr}}};
        output.RegisterBuffers(1, 1, buffers, attribute, 0);
        if (!output.IsBufferReleased(1)) return 68;
        output.MarkBufferPending(1);
        if (output.IsBufferReleased(1)) return 69;
        try
        {
            output.MarkBufferPending(1);
            return 70;
        }
        catch (const std::logic_error&)
        {
        }
        try
        {
            output.WaitForBufferRelease(1, 0);
            return 71;
        }
        catch (const std::runtime_error&)
        {
        }
        output.SubmitFlip(1, 1, 123);
        WaitForFlip(output, 123);
        if (output.GetFlipStatus().currentBuffer != 1) return 67;
        if (output.IsBufferReleased(1)) return 72;
        output.SubmitFlip(-1, 1, 124);
        WaitForFlip(output, 124);
        output.WaitForBufferRelease(1, 5000000000ull);
        if (!output.IsBufferReleased(1)) return 73;
        output.UnregisterBuffers(1);
        output.RegisterBuffers(1, 1, buffers, attribute, 0);
        output.UnregisterBuffers(1);
        output.Close();
        output.Close();
        try
        {
            output.GetHandle();
            return 65;
        }
        catch (const std::logic_error&)
        {
        }
    }
    return 0;
}
