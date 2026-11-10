#ifndef ANYUNREAL_VIDEO_OUTPUT_H
#define ANYUNREAL_VIDEO_OUTPUT_H

#include "SceTypes.hpp"
#include <optional>
#include <span>

namespace AnyUnreal::Video
{
    class Output final
    {
    public:
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
        void Close();

    private:
        std::optional<int> handle;
    };
}

#endif
