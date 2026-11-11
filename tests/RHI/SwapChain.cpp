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
    auto output = AnyUnreal::Video::Output::AcquirePrimary();
    if (AnyUnreal::Video::Output::AcquirePrimary() != output) throw std::runtime_error("Primary video output was opened more than once");
    const auto resolution = output->GetResolution();
    if (resolution.fullWidth == 0 || resolution.fullHeight == 0 || resolution.paneWidth == 0 || resolution.paneHeight == 0) throw std::runtime_error("Primary video output reported an empty mode");
    const auto outputHandle = output->GetHandle();
    SwapChain chain(output, 259, 137, TextureFormat::Bgra8Unorm, 2);
    if (chain.GetHandle() != outputHandle) throw std::runtime_error("Swap chain did not retain the provided video output");
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
                chain.Discard();
                bool discardedRejected = false;
                try { chain.Present(1); }
                catch (const std::logic_error&) { discardedRejected = true; }
                if (!discardedRejected || chain.GetBufferIndex() != 0 || chain.Acquire() != buffer) throw std::runtime_error("Discarded frame changed the swap chain buffer");
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
    static_cast<void>(chain.Acquire());
    chain.Discard();
    chain.Resize(131, 79, TextureFormat::Bgra8Unorm);
    chain.Close();
    chain.Close();
    bool rejected = false;
    try { static_cast<void>(chain.Acquire()); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Closed swap chain returned a frame");
    rejected = false;
    try { chain.Discard(); }
    catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Closed swap chain accepted a discarded frame");
    if (output->GetHandle() != outputHandle) throw std::runtime_error("Closing the swap chain closed its shared video output");
    SwapChain replacement(output, 131, 79, TextureFormat::Bgra8Unorm, 2);
    output.reset();
    if (replacement.GetHandle() != outputHandle) throw std::runtime_error("Replacement swap chain lost the shared video output");
    const auto replacementBuffer = replacement.Acquire();
    BufferAllocation fenceStorage(8, 2);
    auto& fence = *std::construct_at(static_cast<std::uint64_t*>(fenceStorage.Data()), 0);
    CommandStorage storage(65536, 2);
    auto& commands = storage.Get();
    sceAgcDcbContextStateOp(&commands, 0);
    WriteColorClear(commands, *replacementBuffer, {0.5f, 0.25f, 0.75f, 1.0f});
    SubmitAndWait(commands, fence, 1, 10000000);
    replacement.Present(1);
    replacement.Close();
    if (AnyUnreal::Video::Output::AcquirePrimary()->GetHandle() != outputHandle) throw std::runtime_error("Primary video output did not survive viewport replacement");
    std::cout << "Swap chain presented 49 GPU frames, resized and reused its video output safely\n";
    return 0;
}
