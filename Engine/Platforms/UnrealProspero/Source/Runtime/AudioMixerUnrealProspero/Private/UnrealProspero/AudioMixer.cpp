#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Misc/CoreDelegates.h"
#include "UnrealProspero/OutputPort.h"
#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>

namespace
{
    using namespace Audio;
    using UnrealProspero::Audio::OutputPort;

    constexpr auto DeviceName = TEXT("PS5 Main Output");

    class FProsperoMixer final : public Audio::IAudioMixerPlatformInterface
    {
    public:
        FProsperoMixer() : pump(*this)
        {
        }

        ~FProsperoMixer() override
        {
            TeardownHardware();
        }

        FString GetPlatformApi() const override { return TEXT("SceAudioOut"); }
        FString GetDefaultDeviceName() override { return DeviceName; }
        FAudioPlatformSettings GetPlatformSettings() const override { return {}; }
        FAudioPlatformDeviceInfo GetPlatformDeviceInfo() const override { return AudioStreamInfo.DeviceInfo; }

        bool InitializeHardware() override
        {
            if (IsInitialized()) return true;
            enteringBackground = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddRaw(this, &FProsperoMixer::SuspendContext);
            enteringForeground = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddRaw(this, &FProsperoMixer::ResumeContext);
            bIsDeviceInitialized = true;
            return true;
        }

        bool TeardownHardware() override
        {
            if (!CloseAudioStream()) return false;
            FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(enteringBackground);
            FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(enteringForeground);
            enteringBackground.Reset();
            enteringForeground.Reset();
            bIsDeviceInitialized = false;
            return true;
        }

        bool IsInitialized() const override { return bIsDeviceInitialized; }

        bool GetOutputDeviceInfo(uint32 index, FAudioPlatformDeviceInfo& info) override
        {
            if (index != 0) return false;
            info.Name = DeviceName;
            info.DeviceId = DeviceName;
            info.NumChannels = OutputPort::Channels;
            info.NumDirectOutChannels = 0;
            info.SampleRate = OutputPort::SampleRate;
            info.Format = EAudioMixerStreamDataFormat::Float;
            info.OutputChannelArray = {EAudioMixerChannel::FrontLeft, EAudioMixerChannel::FrontRight};
            info.bIsSystemDefault = true;
            return true;
        }

        bool OpenAudioStream(const FAudioMixerOpenStreamParams& parameters) override
        {
            if (!IsInitialized() || AudioStreamInfo.StreamState != EAudioOutputStreamState::Closed) return false;
            if ((parameters.OutputDeviceIndex != 0 && parameters.OutputDeviceIndex != static_cast<uint32>(INDEX_NONE)) || !parameters.AudioMixer || parameters.SampleRate != OutputPort::SampleRate) return false;
            if (!parameters.bUseSystemAudioDevice && parameters.AudioDeviceId != DeviceName) return false;
            port = std::make_unique<OutputPort>(parameters.NumFrames);
            OpenStreamParams = parameters;
            AudioStreamInfo.Reset();
            GetOutputDeviceInfo(0, AudioStreamInfo.DeviceInfo);
            AudioStreamInfo.OutputDeviceIndex = 0;
            AudioStreamInfo.AudioMixer = parameters.AudioMixer;
            AudioStreamInfo.NumOutputFrames = parameters.NumFrames;
            AudioStreamInfo.NumBuffers = parameters.NumBuffers;
            AudioStreamInfo.StreamState = EAudioOutputStreamState::Open;
            return true;
        }

        bool CloseAudioStream() override
        {
            if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Closed) return true;
            if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Running && !StopAudioStream()) return false;
            port->Close();
            port.reset();
            restartOnResume = false;
            AudioStreamInfo.Reset();
            return true;
        }

        bool StartAudioStream() override
        {
            if (restartOnResume || !port || (AudioStreamInfo.StreamState != EAudioOutputStreamState::Open && AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped)) return false;
            primed.store(false, std::memory_order_relaxed);
            running.store(true, std::memory_order_release);
            BeginGeneratingAudio();
            thread.Reset(FRunnableThread::Create(&pump, TEXT("PS5AudioOutput"), 0, TPri_AboveNormal));
            if (thread) return true;
            running.store(false, std::memory_order_release);
            StopGeneratingAudio();
            return false;
        }

        bool StopAudioStream() override
        {
            restartOnResume = false;
            if (AudioStreamInfo.StreamState == EAudioOutputStreamState::Stopped) return true;
            if (AudioStreamInfo.StreamState != EAudioOutputStreamState::Running || !thread) return false;
            running.store(false, std::memory_order_release);
            thread->WaitForCompletion();
            thread.Reset();
            StopGeneratingAudio();
            return true;
        }

        void SuspendContext() override
        {
            if (AudioStreamInfo.StreamState != EAudioOutputStreamState::Running) return;
            if (!StopAudioStream()) throw std::runtime_error("Cannot suspend PS5 audio output");
            restartOnResume = true;
        }

        void ResumeContext() override
        {
            if (!restartOnResume) return;
            restartOnResume = false;
            if (!StartAudioStream()) throw std::runtime_error("Cannot resume PS5 audio output");
        }

        void SubmitBuffer(const uint8* buffer) override
        {
            try
            {
                port->Submit({reinterpret_cast<const float*>(buffer), static_cast<std::size_t>(AudioStreamInfo.NumOutputFrames) * OutputPort::Channels});
                primed.store(true, std::memory_order_release);
            }
            catch (const std::exception& error)
            {
                UE_LOGF(LogAudioMixer, Fatal, "PS5 audio submission failed: %hs", error.what());
            }
        }

    private:
        class FPump final : public FRunnable
        {
        public:
            explicit FPump(FProsperoMixer& owner) : owner(owner)
            {
            }

            uint32 Run() override
            {
                while (owner.running.load(std::memory_order_acquire))
                {
                    if (owner.primed.load(std::memory_order_acquire)) owner.ReadNextBuffer();
                    else FPlatformProcess::SleepNoStats(0.001f);
                }
                return 0;
            }

        private:
            FProsperoMixer& owner;
        };

        FDelegateHandle enteringBackground;
        FDelegateHandle enteringForeground;
        std::unique_ptr<OutputPort> port;
        std::atomic<bool> running{false};
        std::atomic<bool> primed{false};
        bool restartOnResume = false;
        FPump pump;
        TUniquePtr<FRunnableThread> thread;
    };

    class FProsperoAudioModule final : public IAudioDeviceModule
    {
    public:
        bool IsAudioMixerModule() const override { return true; }
        FAudioDevice* CreateAudioDevice() override { return new Audio::FMixerDevice(CreateAudioMixerPlatformInterface()); }
        Audio::IAudioMixerPlatformInterface* CreateAudioMixerPlatformInterface() override { return new FProsperoMixer(); }
    };
}

IMPLEMENT_MODULE(FProsperoAudioModule, AudioMixerUnrealProspero)
