#ifndef ANYUNREAL_KERNEL_FILE_LOCK_H
#define ANYUNREAL_KERNEL_FILE_LOCK_H

#include "AnyUnreal/Kernel/Descriptor.h"
#include <cstdint>

namespace AnyUnreal::Kernel
{
    class FileLock final
    {
    public:
        explicit FileLock(std::string_view path, std::uint64_t timeoutNanoseconds = 0);
        ~FileLock() noexcept(false);
        FileLock(const FileLock&) = delete;
        FileLock& operator=(const FileLock&) = delete;
        bool IsOwned() const noexcept { return owned; }
        void Release();

    private:
        Descriptor descriptor;
        bool owned = false;
    };
}

#endif
