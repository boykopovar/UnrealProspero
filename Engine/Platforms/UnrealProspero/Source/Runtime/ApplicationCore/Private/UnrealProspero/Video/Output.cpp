#include "UnrealProspero/Video/Output.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceVideoOut/include/Output.hpp"
#include "prx/libSceVideoOut/include/Buffer.hpp"
#include "prx/libSceVideoOut/include/Event.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "UnrealProspero/Kernel/Clock.h"
#include "prx/libkernel/Time/include/Time.hpp"
#include <atomic>
#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace UnrealProspero::Video
{
    namespace
    {
        class OutputModeEvents final
        {
        public:
            explicit OutputModeEvents(int handle) : handle(handle)
            {
                Kernel::CheckResult(sceKernelCreateEqueue(&queue, "UnrealProspero output mode"), "sceKernelCreateEqueue");
                const auto result = sceVideoOutAddOutputModeEvent(queue, handle, nullptr);
                if (result != 0)
                {
                    Kernel::CheckResult(sceKernelDeleteEqueue(queue), "sceKernelDeleteEqueue");
                    throw Kernel::Error(result, "sceVideoOutAddOutputModeEvent");
                }
            }

            ~OutputModeEvents() noexcept(false)
            {
                Kernel::CheckResult(sceVideoOutDeleteOutputModeEvent(queue, handle), "sceVideoOutDeleteOutputModeEvent");
                Kernel::CheckResult(sceKernelDeleteEqueue(queue), "sceKernelDeleteEqueue");
            }

            OutputModeEvents(const OutputModeEvents&) = delete;
            OutputModeEvents& operator=(const OutputModeEvents&) = delete;

            bool Wait(KernelUseconds timeout)
            {
                KernelEvent event{};
                int count = 0;
                const auto result = sceKernelWaitEqueue(queue, &event, 1, &count, &timeout);
                if (result == SCE_KERNEL_ERROR_ETIMEDOUT && timeout == 0) return false;
                Kernel::CheckResult(result, "sceKernelWaitEqueue");
                if (count != 1) throw std::runtime_error("Invalid output mode event count");
                return true;
            }

        private:
            KernelEqueue queue{};
            int handle;
        };
    }

    std::shared_ptr<Output> Output::AcquirePrimary()
    {
        static const auto output = std::make_shared<Output>();
        output->GetHandle();
        return output;
    }

    Output::Output(int userId)
    {
        const int opened = sceVideoOutOpen(userId, 0, 0, nullptr);
        if (opened < 0) throw Kernel::Error(opened, "sceVideoOutOpen");
        handle = opened;
    }

    Output::~Output() noexcept(false)
    {
        Close();
    }

    int Output::GetHandle() const
    {
        if (!handle) throw std::logic_error("Video output is closed");
        return *handle;
    }

    VideoOutResolutionStatus Output::GetResolution() const
    {
        VideoOutResolutionStatus status{};
        Kernel::CheckResult(sceVideoOutGetResolutionStatus(GetHandle(), &status), "sceVideoOutGetResolutionStatus");
        if (status.fullWidth == 0 || status.fullHeight == 0 || status.paneWidth == 0 || status.paneHeight == 0) throw std::runtime_error("Video output reported an empty resolution");
        return status;
    }

    void Output::SetFlipRate(int rate)
    {
        if (rate < 0 || rate > 2) throw std::invalid_argument("Invalid video flip rate");
        Kernel::CheckResult(sceVideoOutSetFlipRate(GetHandle(), rate), "sceVideoOutSetFlipRate");
    }

    void Output::ConfigureHighRefresh()
    {
        VideoOutOutputOptions options{};
        Kernel::CheckResult(sceVideoOutInitializeOutputOptions(&options), "sceVideoOutInitializeOutputOptions");
        const auto supported = sceVideoOutIsOutputSupported(GetHandle(), 15, &options, nullptr, 0);
        if (supported < 0) throw Kernel::Error(supported, "sceVideoOutIsOutputSupported");
        if (supported != 1) throw std::runtime_error("120 Hz output is not supported");
        VideoOutOutputStatus status{};
        Kernel::CheckResult(sceVideoOutGetOutputStatus(GetHandle(), &status), "sceVideoOutGetOutputStatus");
        if (status.refreshRate != 13)
        {
            OutputModeEvents events(GetHandle());
            while (events.Wait(0)) {}
            Kernel::CheckResult(sceVideoOutConfigureOutput(GetHandle(), 15, &options, nullptr, 0), "sceVideoOutConfigureOutput");
            const auto start = Kernel::Clock::MonotonicNanoseconds();
            do
            {
                const auto now = Kernel::Clock::MonotonicNanoseconds();
                if (now < start || now - start >= 5000000000ull) throw std::runtime_error("120 Hz output transition timed out");
                events.Wait(static_cast<KernelUseconds>((5000000000ull - (now - start) + 999) / 1000));
                Kernel::CheckResult(sceVideoOutGetOutputStatus(GetHandle(), &status), "sceVideoOutGetOutputStatus");
            } while (status.refreshRate != 13);
        }
        SetFlipRate(0);
    }

    void Output::WaitVblank()
    {
        Kernel::CheckResult(sceVideoOutWaitVblank(GetHandle()), "sceVideoOutWaitVblank");
    }

    void Output::RegisterBuffers(int setIndex, int firstBuffer, std::span<const VideoOutBuffers> buffers, const VideoOutBufferAttribute2& attribute, int category)
    {
        if (buffers.empty() || buffers.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::invalid_argument("Invalid video buffer count");
        const int result = sceVideoOutRegisterBuffers2(GetHandle(), setIndex, firstBuffer, buffers.data(), static_cast<int>(buffers.size()), &attribute, category, nullptr);
        if (result != 0)
        {
            const auto operation = "sceVideoOutRegisterBuffers2(width=" + std::to_string(attribute.width) + ", height=" + std::to_string(attribute.height) + ", pitch=" + std::to_string(attribute.pitch_in_pixel) + ", tiling=" + std::to_string(attribute.tiling_mode) + ", format=" + std::to_string(attribute.pixel_format) + ")";
            throw Kernel::Error(result, operation.c_str());
        }
    }

    void Output::UnregisterBuffers(int setIndex)
    {
        Kernel::CheckResult(sceVideoOutUnregisterBuffers(GetHandle(), setIndex), "sceVideoOutUnregisterBuffers");
    }

    void Output::SubmitFlip(int bufferIndex, int mode, std::int64_t argument)
    {
        Kernel::CheckResult(sceVideoOutSubmitFlip(GetHandle(), bufferIndex, mode, argument), "sceVideoOutSubmitFlip");
    }

    std::uint32_t Output::ReserveEopFlip(int bufferIndex, int mode, std::int64_t argument)
    {
        alignas(std::uint64_t) std::array<std::uint32_t, 2> token{};
        Kernel::CheckResult(sceVideoOutSubmitEopFlip(GetHandle(), bufferIndex, mode, argument, token.data()), "sceVideoOutSubmitEopFlip");
        return token[0] & 0x0fffffffu;
    }

    VideoOutFlipStatus Output::GetFlipStatus() const
    {
        VideoOutFlipStatus status{};
        Kernel::CheckResult(sceVideoOutGetFlipStatus(GetHandle(), &status), "sceVideoOutGetFlipStatus");
        return status;
    }

    std::uint64_t& Output::GetBufferLabel(int bufferIndex) const
    {
        std::uintptr_t address = 0;
        const int count = sceVideoOutGetBufferLabelAddress(GetHandle(), &address);
        if (count < 0) throw Kernel::Error(count, "sceVideoOutGetBufferLabelAddress");
        if (count == 0 || address == 0 || address % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw std::runtime_error("Invalid video buffer labels");
        if (bufferIndex < 0 || bufferIndex >= count) throw std::out_of_range("Invalid video buffer label index");
        return reinterpret_cast<std::uint64_t*>(address)[bufferIndex];
    }

    bool Output::IsBufferReleased(int bufferIndex) const
    {
        std::atomic_ref label(GetBufferLabel(bufferIndex));
        static_assert(decltype(label)::is_always_lock_free);
        return label.load(std::memory_order_acquire) == 0;
    }

    void Output::MarkBufferPending(int bufferIndex)
    {
        std::atomic_ref label(GetBufferLabel(bufferIndex));
        std::uint64_t expected = 0;
        if (!label.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) throw std::logic_error("Video buffer is still pending");
    }

    void Output::WaitForBufferRelease(int bufferIndex, std::uint64_t timeoutNanoseconds) const
    {
        const auto start = Kernel::Clock::MonotonicNanoseconds();
        while (!IsBufferReleased(bufferIndex))
        {
            const auto now = Kernel::Clock::MonotonicNanoseconds();
            if (now < start) throw std::runtime_error("Monotonic clock moved backwards");
            if (now - start >= timeoutNanoseconds) throw std::runtime_error("Video buffer release timed out");
            Kernel::CheckResult(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
        }
    }

    void Output::Close()
    {
        if (!handle) return;
        Kernel::CheckResult(sceVideoOutClose(*handle), "sceVideoOutClose");
        handle.reset();
    }
}
