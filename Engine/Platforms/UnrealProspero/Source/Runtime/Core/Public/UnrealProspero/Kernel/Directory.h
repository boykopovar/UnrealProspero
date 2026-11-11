#ifndef UNREALPROSPERO_KERNEL_DIRECTORY_H
#define UNREALPROSPERO_KERNEL_DIRECTORY_H

#include "UnrealProspero/Kernel/Descriptor.h"
#include "prx/libc/include/GuestDirectory.hpp"
#include <array>
#include <optional>

namespace UnrealProspero::Kernel
{
    class Directory final
    {
    public:
        explicit Directory(std::string_view path);
        ~Directory() noexcept(false) = default;
        Directory(const Directory&) = delete;
        Directory& operator=(const Directory&) = delete;
        std::optional<GuestDirectoryEntry> Next();

    private:
        Descriptor descriptor;
        std::array<char, 4096> buffer{};
        std::size_t offset = 0;
        std::size_t available = 0;
        bool finished = false;
    };
}

#endif
