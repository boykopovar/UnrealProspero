#include "UnrealProspero/Kernel/Directory.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstring>

namespace UnrealProspero::Kernel
{
    Directory::Directory(std::string_view path) : descriptor(Descriptor::Open(path, 0x20000)), buffer(64 * 1024)
    {
    }

    std::optional<GuestDirectoryEntry> Directory::Next()
    {
        constexpr std::size_t headerSize = offsetof(GuestDirectoryEntry, name);
        while (!finished)
        {
            if (offset == available)
            {
                std::int64_t position = 0;
                const int count = sceKernelGetdirentries(descriptor.Get(), buffer.data(), static_cast<int>(buffer.size()), &position);
                if (count == SCE_KERNEL_ERROR_EINTR) continue;
                if (count < 0) throw Error(count, "sceKernelGetdirentries");
                if (static_cast<std::size_t>(count) > buffer.size()) throw std::runtime_error("Directory read exceeded its buffer");
                available = static_cast<std::size_t>(count);
                offset = 0;
                if (available == 0)
                {
                    finished = true;
                    return std::nullopt;
                }
            }
            if (available - offset < headerSize) throw std::runtime_error("Truncated directory entry header");
            GuestDirectoryEntry entry{};
            std::memcpy(&entry, buffer.data() + offset, headerSize);
            const auto length = static_cast<std::size_t>(entry.recordLength);
            if (length < headerSize + 1 || length > available - offset || length % 4 != 0) throw std::runtime_error("Invalid directory entry length");
            const auto* nameData = buffer.data() + offset + headerSize;
            offset += length;
            if (entry.fileNumber == 0) continue;
            if (entry.nameLength == 0 || headerSize + entry.nameLength + 1 > length) throw std::runtime_error("Invalid directory entry name length");
            std::memcpy(entry.name, nameData, entry.nameLength + 1u);
            if (entry.name[entry.nameLength] != '\0' || std::memchr(entry.name, '\0', entry.nameLength) != nullptr || std::memchr(entry.name, '/', entry.nameLength) != nullptr) throw std::runtime_error("Invalid directory entry name");
            const std::string_view name(entry.name, entry.nameLength);
            if (name == "." || name == "..") continue;
            return entry;
        }
        return std::nullopt;
    }
}
