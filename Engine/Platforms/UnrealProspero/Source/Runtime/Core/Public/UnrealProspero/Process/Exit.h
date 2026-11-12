#ifndef UNREALPROSPERO_PROCESS_EXIT_H
#define UNREALPROSPERO_PROCESS_EXIT_H

#include <cstdint>

namespace UnrealProspero::Process
{
    class Exit final
    {
    public:
        static void SetStatus(std::uint8_t status) noexcept;
        static bool GetStatus(std::uint8_t* status) noexcept;
        static int Complete(int status);
        [[noreturn]] static void Force();
    };
}

#endif
