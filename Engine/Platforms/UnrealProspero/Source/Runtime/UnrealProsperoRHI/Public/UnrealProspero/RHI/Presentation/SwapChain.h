#ifndef UNREALPROSPERO_RHI_SWAP_CHAIN_H
#define UNREALPROSPERO_RHI_SWAP_CHAIN_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include "UnrealProspero/Video/Output.h"
#include <memory>

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
