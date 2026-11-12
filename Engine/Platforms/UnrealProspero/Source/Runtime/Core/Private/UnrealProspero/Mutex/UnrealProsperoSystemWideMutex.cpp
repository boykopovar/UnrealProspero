#include "UnrealProspero/Mutex/UnrealProsperoSystemWideMutex.h"
#include "Containers/UnrealString.h"
#include "UnrealProspero/Text/Encoding.h"
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    std::string lockPath(const FString& name)
    {
        if (name.IsEmpty()) throw std::invalid_argument("Empty system mutex name");
        const std::string encoded = UnrealProspero::Text::EncodeUtf8(std::basic_string_view<TCHAR>(*name, name.Len()));
        constexpr char digits[] = "0123456789abcdef";
        std::string path = "/tmp/UnrealProsperoMutex-";
        for (unsigned char character : encoded)
        {
            path.push_back(digits[character >> 4]);
            path.push_back(digits[character & 15]);
        }
        return path;
    }

    std::uint64_t timeoutNanoseconds(FTimespan timeout)
    {
        if (timeout.GetTicks() < 0) throw std::invalid_argument("Negative system mutex timeout");
        const auto ticks = static_cast<std::uint64_t>(timeout.GetTicks());
        if (ticks > std::numeric_limits<std::uint64_t>::max() / 100) throw std::overflow_error("System mutex timeout exceeds clock range");
        return ticks * 100;
    }
}

namespace UE
{
    FUnrealProsperoSystemWideMutex::FUnrealProsperoSystemWideMutex(const FString& name, FTimespan timeout) : lock(lockPath(name), timeoutNanoseconds(timeout)) {}

    void FUnrealProsperoSystemWideMutex::Release()
    {
        lock.Release();
    }
}
