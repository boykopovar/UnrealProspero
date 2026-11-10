#ifndef ANYUNREAL_KERNEL_FILE_H
#define ANYUNREAL_KERNEL_FILE_H

#include "AnyUnreal/Kernel/Descriptor.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace AnyUnreal::Kernel
{
    enum class FileAccess { Read, Write, ReadWrite };
    enum class FileCreation { Existing, OpenOrCreate, CreateNew, TruncateExisting, CreateOrTruncate };

    class File final
    {
    public:
        File(std::string_view path, FileAccess access, FileCreation creation = FileCreation::Existing, bool append = false);
        ~File() noexcept(false) = default;
        File(const File&) = delete;
        File& operator=(const File&) = delete;
        std::size_t Read(std::span<std::byte> destination);
        std::size_t ReadAt(std::span<std::byte> destination, std::int64_t offset);
        void Write(std::span<const std::byte> source);
        std::int64_t Tell() const;
        void Seek(std::int64_t position);
        void SeekFromEnd(std::int64_t offset = 0);
        std::int64_t Size() const;
        void Flush();
        void Truncate(std::int64_t size);
        void Close();

    private:
        Descriptor descriptor;
        std::size_t read(std::span<std::byte> destination, std::int64_t offset, bool positioned);
    };
}

#endif
