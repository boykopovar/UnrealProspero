#ifndef UNREAL_PROSPERO_AUDIO_OUTPUT_PORT_H
#define UNREAL_PROSPERO_AUDIO_OUTPUT_PORT_H

#include <cstdint>
#include <span>

namespace UnrealProspero::Audio
{
    class OutputPort final
    {
    public:
        static constexpr std::uint32_t SampleRate = 48000;
        static constexpr std::uint32_t Channels = 2;

        explicit OutputPort(std::uint32_t frames);
        ~OutputPort();
        OutputPort(const OutputPort&) = delete;
        OutputPort& operator=(const OutputPort&) = delete;

        void Submit(std::span<const float> samples) const;
        void Close();

    private:
        int handle;
        std::uint32_t frames;
    };
}

#endif
