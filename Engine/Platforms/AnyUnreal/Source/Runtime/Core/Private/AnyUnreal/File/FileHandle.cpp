#include "AnyUnreal/File/FileHandle.h"
#include <stdexcept>

namespace
{
    template<typename TByte>
    std::span<TByte> checkedBuffer(TByte* data, int64 size)
    {
        if (size < 0 || (data == nullptr && size != 0)) throw std::invalid_argument("Invalid file transfer buffer");
        return {data, static_cast<std::size_t>(size)};
    }
}

FAnyUnrealFileHandle::FAnyUnrealFileHandle(std::string_view path, AnyUnreal::Kernel::FileAccess access, AnyUnreal::Kernel::FileCreation creation, bool append) : file(path, access, creation, append)
{
}

int64 FAnyUnrealFileHandle::Tell()
{
    return file.Tell();
}

bool FAnyUnrealFileHandle::Seek(int64 position)
{
    file.Seek(position);
    return true;
}

bool FAnyUnrealFileHandle::SeekFromEnd(int64 offset)
{
    file.SeekFromEnd(offset);
    return true;
}

bool FAnyUnrealFileHandle::Read(uint8* destination, int64 size)
{
    auto buffer = std::as_writable_bytes(checkedBuffer(destination, size));
    return file.Read(buffer) != 0 || size == 0;
}

bool FAnyUnrealFileHandle::ReadAt(uint8* destination, int64 size, int64 offset)
{
    auto buffer = std::as_writable_bytes(checkedBuffer(destination, size));
    return file.ReadAt(buffer, offset) == buffer.size();
}

bool FAnyUnrealFileHandle::Write(const uint8* source, int64 size)
{
    file.Write(std::as_bytes(checkedBuffer(source, size)));
    return true;
}

bool FAnyUnrealFileHandle::Flush(bool)
{
    file.Flush();
    return true;
}

bool FAnyUnrealFileHandle::Truncate(int64 size)
{
    file.Truncate(size);
    return true;
}

int64 FAnyUnrealFileHandle::Size()
{
    return file.Size();
}
