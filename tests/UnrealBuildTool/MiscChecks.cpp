#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include "HAL/PlatformMisc.h"
#include "UnrealProspero/Text/Encoding.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>
#include <atomic>
#include <stdexcept>

namespace
{
    std::atomic<int> receivedSignal{0};
    static_assert(std::atomic<int>::is_always_lock_free);

    void APS5_VABI receiveSignal(int signal)
    {
        receivedSignal.store(signal, std::memory_order_relaxed);
    }
}

bool CheckMisc()
{
    using UnrealProspero::Kernel::SystemConfiguration;
    const auto processors = SystemConfiguration::LogicalProcessorCount();
    const auto pageSize = SystemConfiguration::PageSize();
    if (processors != sysconf_nid_postfix(58) || processors != FPlatformMisc::NumberOfCoresIncludingHyperthreads()) return false;
    if (pageSize != static_cast<std::size_t>(getpagesize_nid_postfix())) return false;
    const auto pages = sysconf_nid_postfix(121);
    if (pages <= 0 || SystemConfiguration::PhysicalMemorySize() != static_cast<std::uint64_t>(pages) * pageSize) return false;
    if (UnrealProspero::Text::EncodeUtf8(TEXT("A\u0411\u20ac\U0001f642")) != "A\xd0\x91\xe2\x82\xac\xf0\x9f\x99\x82") return false;
    const std::array<TCHAR, 2> boundary{static_cast<TCHAR>(0xdbff), static_cast<TCHAR>(0xdfff)};
    if (UnrealProspero::Text::EncodeUtf8({boundary.data(), boundary.size()}) != "\xf4\x8f\xbf\xbf") return false;
    const std::array<TCHAR, 3> embedded{TEXT('A'), 0, TEXT('B')};
    if (UnrealProspero::Text::EncodeUtf8({embedded.data(), embedded.size()}) != std::string("A\0B", 3)) return false;
    const std::array<std::basic_string<TCHAR>, 3> invalid{{{static_cast<TCHAR>(0xd800)}, {static_cast<TCHAR>(0xdc00)}, {static_cast<TCHAR>(0xd800), TEXT('A')}}};
    for (const auto& text : invalid)
    {
        try
        {
            UnrealProspero::Text::EncodeUtf8(text);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    FPlatformMisc::MemoryBarrier();
    FPlatformMisc::LowLevelOutputDebugString(TEXT("UnrealProspero diagnostic output: \u0411\u20ac\U0001f642\n"));
    GuestSigaction action{};
    GuestSigaction previous{};
    action.handler = reinterpret_cast<std::uintptr_t>(&receiveSignal);
    if (sigaction_nid_postfix(5, &action, &previous) != 0) return false;
    FPlatformMisc::DebugBreak();
    if (sigaction_nid_postfix(5, &previous, nullptr) != 0) return false;
    return receivedSignal.load(std::memory_order_relaxed) == 5;
}
