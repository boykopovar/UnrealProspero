#include "AnyUnreal/Kernel/File.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace AnyUnreal::Kernel
{
    namespace
    {
        int openFlags(FileAccess access, FileCreation creation, bool append)
        {
            int flags;
            switch (access)
            {
            case FileAccess::Read: flags = 0; break;
            case FileAccess::Write: flags = 1; break;
            case FileAccess::ReadWrite: flags = 2; break;
            default: throw std::invalid_argument("Invalid file access mode");
            }
            switch (creation)
            {
            case FileCreation::Existing: break;
            case FileCreation::OpenOrCreate: flags |= 0x200; break;
            case FileCreation::CreateNew: flags |= 0xa00; break;
            case FileCreation::TruncateExisting: flags |= 0x400; break;
            case FileCreation::CreateOrTruncate: flags |= 0x600; break;
            default: throw std::invalid_argument("Invalid file creation mode");
            }
            if (append)
            {
                if (access == FileAccess::Read) throw std::invalid_argument("Read-only file cannot append");
                flags |= 0x8;
            }
            return flags;
        }

        std::int64_t checked(std::int64_t result, const char* operation)
        {
            if (result < 0) throw std::runtime_error(std::string(operation) + ": " + std::to_string(result));
            return result;
        }

        void validateSize(std::size_t size)
        {
            if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) throw std::overflow_error("File transfer exceeds signed offset range");
        }

        std::size_t chunkSize(std::size_t remaining)
        {
            return std::min(remaining, static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()));
        }
    }

    File::File(std::string_view path, FileAccess access, FileCreation creation, bool append)
    {
        if (path.empty() || path.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid file path");
        const int flags = openFlags(access, creation, append);
        descriptor = static_cast<int>(checked(sceKernelOpen(std::string(path).c_str(), flags, 0666), "sceKernelOpen"));
    }

    File::~File() noexcept(false)
    {
        if (descriptor != -1) Close();
    }

    int File::getDescriptor() const
    {
        if (descriptor < 0) throw std::logic_error("File is closed");
        return descriptor;
    }

    void File::Close()
    {
        getDescriptor();
        CheckResult(sceKernelClose(std::exchange(descriptor, -1)), "sceKernelClose");
    }

    std::size_t File::read(std::span<std::byte> destination, std::int64_t offset, bool positioned)
    {
        const int handle = getDescriptor();
        validateSize(destination.size());
        if (positioned && (offset < 0 || destination.size() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() - offset))) throw std::invalid_argument("Invalid positioned read range");
        std::size_t total = 0;
        while (total < destination.size())
        {
            const auto count = chunkSize(destination.size() - total);
            const auto result = positioned ? sceKernelPread(handle, destination.data() + total, count, offset + static_cast<std::int64_t>(total)) : sceKernelRead(handle, destination.data() + total, count);
            if (result == SCE_KERNEL_ERROR_EINTR) continue;
            checked(result, "file read");
            if (static_cast<std::uint64_t>(result) > count) throw std::runtime_error("File read exceeded the requested size");
            if (result == 0) break;
            total += static_cast<std::size_t>(result);
        }
        return total;
    }

    std::size_t File::Read(std::span<std::byte> destination)
    {
        return read(destination, 0, false);
    }

    std::size_t File::ReadAt(std::span<std::byte> destination, std::int64_t offset)
    {
        return read(destination, offset, true);
    }

    void File::Write(std::span<const std::byte> source)
    {
        const int handle = getDescriptor();
        validateSize(source.size());
        std::size_t total = 0;
        while (total < source.size())
        {
            const auto count = chunkSize(source.size() - total);
            const auto result = sceKernelWrite(handle, source.data() + total, count);
            if (result == SCE_KERNEL_ERROR_EINTR) continue;
            checked(result, "sceKernelWrite");
            if (result == 0 || static_cast<std::uint64_t>(result) > count) throw std::runtime_error("Invalid file write progress");
            total += static_cast<std::size_t>(result);
        }
    }

    std::int64_t File::Tell() const
    {
        return checked(sceKernelLseek(getDescriptor(), 0, 1), "sceKernelLseek");
    }

    void File::Seek(std::int64_t position)
    {
        if (position < 0) throw std::invalid_argument("Negative file position");
        checked(sceKernelLseek(getDescriptor(), position, 0), "sceKernelLseek");
    }

    void File::SeekFromEnd(std::int64_t offset)
    {
        if (offset > 0) throw std::invalid_argument("File end offset must not be positive");
        checked(sceKernelLseek(getDescriptor(), offset, 2), "sceKernelLseek");
    }

    std::int64_t File::Size() const
    {
        FileStat status{};
        CheckResult(sceKernelFstat(getDescriptor(), &status), "sceKernelFstat");
        return checked(status.st_size, "file size");
    }

    void File::Flush()
    {
        CheckResult(sceKernelFsync(getDescriptor()), "sceKernelFsync");
    }

    void File::Truncate(std::int64_t size)
    {
        if (size < 0) throw std::invalid_argument("Negative file size");
        CheckResult(sceKernelFtruncate(getDescriptor(), size), "sceKernelFtruncate");
    }
}
