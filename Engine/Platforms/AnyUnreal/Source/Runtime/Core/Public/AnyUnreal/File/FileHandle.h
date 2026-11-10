#ifndef ANYUNREAL_FILE_FILE_HANDLE_H
#define ANYUNREAL_FILE_FILE_HANDLE_H

#include "GenericPlatform/GenericPlatformFile.h"
#include "AnyUnreal/Kernel/File.h"

class FAnyUnrealFileHandle final : public IFileHandle
{
public:
    CORE_API FAnyUnrealFileHandle(std::string_view path, AnyUnreal::Kernel::FileAccess access, AnyUnreal::Kernel::FileCreation creation = AnyUnreal::Kernel::FileCreation::Existing, bool append = false);
    ~FAnyUnrealFileHandle() noexcept override = default;
    FAnyUnrealFileHandle(const FAnyUnrealFileHandle&) = delete;
    FAnyUnrealFileHandle& operator=(const FAnyUnrealFileHandle&) = delete;
    CORE_API int64 Tell() override;
    CORE_API bool Seek(int64 position) override;
    CORE_API bool SeekFromEnd(int64 offset = 0) override;
    CORE_API bool Read(uint8* destination, int64 size) override;
    CORE_API bool ReadAt(uint8* destination, int64 size, int64 offset) override;
    CORE_API bool Write(const uint8* source, int64 size) override;
    CORE_API bool Flush(bool fullFlush = false) override;
    CORE_API bool Truncate(int64 size) override;
    CORE_API int64 Size() override;

private:
    AnyUnreal::Kernel::File file;
};

#endif
