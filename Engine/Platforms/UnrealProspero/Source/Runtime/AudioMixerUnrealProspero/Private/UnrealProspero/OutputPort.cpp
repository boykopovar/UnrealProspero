#include "UnrealProspero/OutputPort.h"
#include "prx/libSceAudioOut/include/AudioOut.hpp"
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>

namespace UnrealProspero::Audio
{
    namespace
    {
        constexpr int SystemUser = 0x10000000;
        constexpr int MainPort = 0;
        constexpr std::uint32_t FloatStereo = 4;
        constexpr std::uint32_t AlreadyInitialized = 0x8026000e;

        void RequireSuccess(int result, const char* operation)
        {
            if (result < 0) throw std::runtime_error(std::string(operation) + " failed with " + std::to_string(result));
        }

        int OpenPort(std::uint32_t frames)
        {
            if (frames < 256 || frames > 2048 || (frames & (frames - 1)) != 0) throw std::invalid_argument("Invalid PS5 audio frame count");
            const int initialized = sceAudioOutInit();
            if (static_cast<std::uint32_t>(initialized) != AlreadyInitialized) RequireSuccess(initialized, "sceAudioOutInit");
            const int handle = sceAudioOutOpen(SystemUser, MainPort, 0, frames, OutputPort::SampleRate, FloatStereo);
            RequireSuccess(handle, "sceAudioOutOpen");
            if (handle == 0) throw std::runtime_error("sceAudioOutOpen returned an invalid port");
            return handle;
        }
    }

    OutputPort::OutputPort(std::uint32_t frames) : handle(OpenPort(frames)), frames(frames)
    {
    }

    OutputPort::~OutputPort()
    {
        if (handle > 0 && sceAudioOutClose(handle) < 0) std::terminate();
    }

    void OutputPort::Submit(std::span<const float> samples) const
    {
        if (handle <= 0) throw std::logic_error("PS5 audio port is closed");
        if (samples.size() != static_cast<std::size_t>(frames) * Channels) throw std::invalid_argument("Wrong number of samples for the PS5 audio port");
        RequireSuccess(sceAudioOutOutput(handle, samples.data()), "sceAudioOutOutput");
    }

    void OutputPort::Close()
    {
        if (handle <= 0) return;
        RequireSuccess(sceAudioOutClose(handle), "sceAudioOutClose");
        handle = 0;
    }
}
