#ifndef ANYUNREAL_VIDEO_OUTPUT_H
#define ANYUNREAL_VIDEO_OUTPUT_H

#include "SceTypes.hpp"
#include <optional>

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
        void Close();

    private:
        std::optional<int> handle;
    };
}

#endif
