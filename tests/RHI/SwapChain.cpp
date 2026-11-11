#include "AnyUnreal/RHI/Presentation/SwapChain.h"
#include "AnyUnreal/RHI/Command/ColorClear.h"
#include "AnyUnreal/RHI/Command/BufferFill.h"
#include "AnyUnreal/RHI/Command/CommandStorage.h"
#include "AnyUnreal/RHI/Command/Submission.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceAgc/libSceAgc.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

int main()
{
    using namespace AnyUnreal::RHI;
    AnyUnreal::Kernel::CheckResult(sceAgcInit(11), "sceAgcInit");
    SwapChain chain(259, 137, TextureFormat::Bgra8Unorm, 2);
    for (const auto format : {TextureFormat::Bgra8Unorm, TextureFormat::Rgba8Unorm, TextureFormat::Bgra8Srgb, TextureFormat::Rgba8Srgb})
    {
        chain.Resize(259, 137, format);
        bool rejected = false;
        try { chain.Present(1); }
        catch (const std::logic_error&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Unacquired swap chain frame was presented");
        for (std::uint32_t frame = 0; frame < 12; ++frame)
        {
            const auto buffer = chain.Acquire();
            if (chain.GetBufferIndex() != frame % 3 || chain.Acquire() != buffer) throw std::runtime_error("Swap chain acquisition changed its current buffer");
            if (reinterpret_cast<std::uintptr_t>(buffer->GetStorage().Data()) % 65536 != 0) throw std::runtime_error("Swap chain buffer is not display aligned");
            if (frame == 0)
            {
                bool resizeRejected = false;
                try { chain.Resize(131, 79, format); }
                catch (const std::logic_error&) { resizeRejected = true; }
                if (!resizeRejected || chain.Acquire() != buffer) throw std::runtime_error("Swap chain resized an acquired frame");
                bool modeRejected = false;
                try { chain.Present(0); }
                catch (const std::invalid_argument&) { modeRejected = true; }
                if (!modeRejected || chain.Acquire() != buffer) throw std::runtime_error("Invalid flip mode changed the acquired frame");
            }
            BufferAllocation fenceStorage(8, 2);
            auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
            CommandStorage storage(65536, 2);
            auto& commands = storage.Get();
            sceAgcDcbContextStateOp(&commands, 0);
            WriteColorClear(commands, *buffer, {frame % 2 ? 0.75f : 0.1f, 0.25f, 0.5f, 1.0f});
            WriteBufferFill(commands, buffer->GetStorage().Data(), buffer->GetRowPitch() * (buffer->GetHeight() / 2), 0xfff0f0f0u);
            SubmitAndWait(commands, fence, 1, 10000000);
            chain.Present(1);
        }
    }
    chain.Resize(131, 79, TextureFormat::Bgra8Unorm);
    chain.Close();
    chain.Close();
    bool rejected = false;
    try { static_cast<void>(chain.Acquire()); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Closed swap chain returned a frame");
    std::cout << "Swap chain presented 48 GPU frames and resized safely\n";
    return 0;
}
