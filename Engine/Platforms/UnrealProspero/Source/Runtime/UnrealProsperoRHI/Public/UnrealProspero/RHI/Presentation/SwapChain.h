#ifndef UNREALPROSPERO_RHI_SWAP_CHAIN_H
#define UNREALPROSPERO_RHI_SWAP_CHAIN_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include "UnrealProspero/Video/Output.h"
#include <memory>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API SwapChain final
    {
    public:
        SwapChain(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh);
        SwapChain(std::shared_ptr<Video::Output> output, std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh);
        ~SwapChain() noexcept(false);
        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        std::shared_ptr<Texture2D> Acquire();
        void Present(int mode);
        void Present(CommandBuffer& commands, int mode);
        void Discard();
        void Resize(std::uint32_t width, std::uint32_t height, TextureFormat format);
        void Close();
        int GetHandle() const;
        std::uint32_t GetBufferIndex() const;

    private:
        using Buffers = std::array<std::shared_ptr<Texture2D>, 3>;
        static Buffers createBuffers(std::uint32_t width, std::uint32_t height, TextureFormat format, std::uint32_t addressHigh);
        void registerBuffers();
        void drain();
        void requireOpen() const;
        template<typename TSubmit>
        void present(int mode, TSubmit&& submit)
        {
            requireOpen();
            if (!acquired) throw std::logic_error("Swap chain has no acquired frame");
            if (mode < 1 || mode > 6) throw std::invalid_argument("Invalid video flip mode");
            if (sequence == std::numeric_limits<std::int64_t>::max()) throw std::overflow_error("Video flip sequence exhausted");
            try
            {
                output->MarkBufferPending(static_cast<int>(bufferIndex));
                submit(static_cast<int>(bufferIndex), ++sequence);
                acquired = false;
                bufferIndex = (bufferIndex + 1) % buffers.size();
            }
            catch (...)
            {
                failed = true;
                throw;
            }
        }
        Buffers buffers;
        std::shared_ptr<Video::Output> output;
        std::uint32_t addressHigh;
        std::uint32_t bufferIndex = 0;
        std::int64_t sequence = 0;
        bool acquired = false;
        bool registered = false;
        bool closed = false;
        bool failed = false;
    };
}

#endif
