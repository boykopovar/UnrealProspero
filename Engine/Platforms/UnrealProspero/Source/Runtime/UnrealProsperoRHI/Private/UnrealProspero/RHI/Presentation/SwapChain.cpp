#include "UnrealProspero/RHI/Presentation/SwapChain.h"
#include "UnrealProspero/Kernel/Clock.h"
#include "prx/libSceVideoOut/include/Buffer.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::uint64_t pixelFormat(TextureFormat format)
        {
            switch (format)
            {
            case TextureFormat::Bgra8Unorm:
            case TextureFormat::Bgra8Srgb: return 0x8000000000000000ull;
            case TextureFormat::Rgba8Unorm:
            case TextureFormat::Rgba8Srgb: return 0x8000000022000000ull;
            case TextureFormat::Rgb10A2Unorm: return 0x8100000022000000ull;
            default: throw std::invalid_argument("Swap chain texture format is not implemented");
            }
        }
    }

    SwapChain::Buffers SwapChain::createBuffers(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh)
    {
        static_cast<void>(pixelFormat(format));
        Buffers result;
        for (auto& buffer : result) buffer = std::make_shared<Texture2D>(width, height, format, addressHigh, 65536, 1, AllocationUsage::Texture, 1, Texture2DLayout::RenderTarget64K);
        return result;
    }

    SwapChain::SwapChain(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh) : SwapChain(std::make_shared<Video::Output>(), width, height, format, addressHigh) {}

    SwapChain::SwapChain(std::shared_ptr<Video::Output> output, std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh) : buffers(createBuffers(width, height, format, addressHigh)), output(std::move(output)), addressHigh(addressHigh)
    {
        if (!this->output) throw std::invalid_argument("Swap chain requires a video output");
        static_cast<void>(this->output->GetHandle());
        registerBuffers();
    }

    SwapChain::~SwapChain() noexcept(false) { Close(); }

    void SwapChain::registerBuffers()
    {
        VideoOutBufferAttribute2 attribute{};
        const auto& first = *buffers[0];
        sceVideoOutSetBufferAttribute2(&attribute, pixelFormat(first.GetFormat()), 0, first.GetWidth(), first.GetHeight(), 0, 0, 0);
        std::array<VideoOutBuffers, 3> addresses{};
        for (std::size_t index = 0; index < buffers.size(); ++index) addresses[index].data = buffers[index]->GetStorage().Data();
        output->RegisterBuffers(0, 0, addresses, attribute, 0);
        registered = true;
    }

    void SwapChain::requireOpen() const
    {
        if (closed || failed || !registered) throw std::runtime_error("Swap chain is closed or failed");
    }

    std::shared_ptr<Texture2D> SwapChain::Acquire()
    {
        requireOpen();
        if (!acquired)
        {
            output->WaitForBufferRelease(static_cast<int>(bufferIndex), 5000000000ull);
            acquired = true;
        }
        return buffers[bufferIndex];
    }

    void SwapChain::Present(int mode)
    {
        requireOpen();
        if (!acquired) throw std::logic_error("Swap chain has no acquired frame");
        if (mode < 1 || mode > 6) throw std::invalid_argument("Invalid video flip mode");
        if (sequence == std::numeric_limits<std::int64_t>::max()) throw std::overflow_error("Video flip sequence exhausted");
        try
        {
            output->MarkBufferPending(static_cast<int>(bufferIndex));
            output->SubmitFlip(static_cast<int>(bufferIndex), mode, ++sequence);
            acquired = false;
            bufferIndex = (bufferIndex + 1) % buffers.size();
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void SwapChain::Discard()
    {
        requireOpen();
        acquired = false;
    }

    void SwapChain::drain()
    {
        if (sequence == std::numeric_limits<std::int64_t>::max()) throw std::overflow_error("Video flip sequence exhausted");
        output->SubmitFlip(-1, 1, ++sequence);
        const auto start = Kernel::Clock::MonotonicNanoseconds();
        while (output->GetFlipStatus().flipArg != sequence)
        {
            const auto now = Kernel::Clock::MonotonicNanoseconds();
            if (now < start || now - start >= 5000000000ull) throw std::runtime_error("Swap chain drain timed out");
            output->WaitVblank();
        }
        for (std::size_t index = 0; index < buffers.size(); ++index) output->WaitForBufferRelease(static_cast<int>(index), 5000000000ull);
    }

    void SwapChain::Resize(std::uint32_t width, std::uint32_t height, TextureFormat format)
    {
        requireOpen();
        if (acquired) throw std::logic_error("Cannot resize an acquired swap chain frame");
        auto replacement = createBuffers(width, height, format, addressHigh);
        try
        {
            drain();
            output->UnregisterBuffers(0);
            registered = false;
            buffers = std::move(replacement);
            bufferIndex = 0;
            registerBuffers();
        }
        catch (...)
        {
            failed = true;
            throw;
        }
    }

    void SwapChain::Close()
    {
        if (closed) return;
        if (registered)
        {
            drain();
            output->UnregisterBuffers(0);
            registered = false;
        }
        if (output.use_count() == 1) output->Close();
        output.reset();
        closed = true;
        acquired = false;
    }

    int SwapChain::GetHandle() const { requireOpen(); return output->GetHandle(); }
    std::uint32_t SwapChain::GetBufferIndex() const { requireOpen(); return bufferIndex; }
}
