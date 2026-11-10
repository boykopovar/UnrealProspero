#ifndef ANYUNREAL_PROCESS_EXIT_H
#define ANYUNREAL_PROCESS_EXIT_H

#include <cstdint>

namespace AnyUnreal::Process
{
    class Exit final
    {
    public:
        static void SetStatus(std::uint8_t status) noexcept;
        static bool GetStatus(std::uint8_t* status) noexcept;
        [[noreturn]] static void Force() noexcept;
    };
}

#endif
