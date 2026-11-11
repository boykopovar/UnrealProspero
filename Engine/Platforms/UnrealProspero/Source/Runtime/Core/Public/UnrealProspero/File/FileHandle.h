#ifndef UNREALPROSPERO_FILE_FILE_HANDLE_H
#define UNREALPROSPERO_FILE_FILE_HANDLE_H

#include "GenericPlatform/GenericPlatformFile.h"
#include "UnrealProspero/Kernel/File.h"

class FUnrealProsperoFileHandle final : public IFileHandle
{
public:
    CORE_API FUnrealProsperoFileHandle(std::string_view path, UnrealProspero::Kernel::FileAccess access, UnrealProspero::Kernel::FileCreation creation = UnrealProspero::Kernel::FileCreation::Existing, bool append = false);
    ~FUnrealProsperoFileHandle() noexcept override = default;
    FUnrealProsperoFileHandle(const FUnrealProsperoFileHandle&) = delete;
    FUnrealProsperoFileHandle& operator=(const FUnrealProsperoFileHandle&) = delete;
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
    UnrealProspero::Kernel::File file;
};

#endif
