#ifndef UNREALPROSPERO_VIDEO_OUTPUT_H
#define UNREALPROSPERO_VIDEO_OUTPUT_H

#include "SceTypes.hpp"
#include <optional>
#include <memory>
#include <span>

namespace UnrealProspero::Video
{
    class Output final
    {
    public:
        static std::shared_ptr<Output> AcquirePrimary();
        explicit Output(int userId = 255);
        ~Output() noexcept(false);
        Output(const Output&) = delete;
        Output& operator=(const Output&) = delete;
        int GetHandle() const;
        VideoOutResolutionStatus GetResolution() const;
        void SetFlipRate(int rate);
        void WaitVblank();
        void RegisterBuffers(int setIndex, int firstBuffer, std::span<const VideoOutBuffers> buffers, const VideoOutBufferAttribute2& attribute, int category);
        void UnregisterBuffers(int setIndex);
        void SubmitFlip(int bufferIndex, int mode, std::int64_t argument);
        VideoOutFlipStatus GetFlipStatus() const;
        bool IsBufferReleased(int bufferIndex) const;
        void MarkBufferPending(int bufferIndex);
        void WaitForBufferRelease(int bufferIndex, std::uint64_t timeoutNanoseconds) const;
        void Close();

    private:
        std::uint64_t& getBufferLabel(int bufferIndex) const;
        std::optional<int> handle;
    };
}

#endif
