#ifndef ANYUNREAL_PLATFORM_FILE_H
#define ANYUNREAL_PLATFORM_FILE_H

#include "GenericPlatform/GenericPlatformFile.h"

class FAnyUnrealPlatformFile final : public IPhysicalPlatformFile
{
public:
    using IPhysicalPlatformFile::IterateDirectory;
    using IPhysicalPlatformFile::IterateDirectoryStat;
    using IPhysicalPlatformFile::OpenWrite;
    CORE_API bool Initialize(IPlatformFile* inner, const TCHAR* commandLine) override;
    CORE_API void SetLowerLevel(IPlatformFile* lowerLevel) override;
    CORE_API bool FileExists(const TCHAR* filename) override;
    CORE_API int64 FileSize(const TCHAR* filename) override;
    CORE_API bool DeleteFile(const TCHAR* filename) override;
    CORE_API bool IsReadOnly(const TCHAR* filename) override;
    CORE_API bool MoveFile(const TCHAR* destination, const TCHAR* source) override;
    CORE_API bool SetReadOnly(const TCHAR* filename, bool readOnly) override;
    CORE_API FDateTime GetTimeStamp(const TCHAR* filename) override;
    CORE_API void SetTimeStamp(const TCHAR* filename, FDateTime time) override;
    CORE_API FDateTime GetAccessTimeStamp(const TCHAR* filename) override;
    CORE_API FString GetFilenameOnDisk(const TCHAR* filename) override;
    CORE_API IFileHandle* OpenRead(const TCHAR* filename, bool allowWrite = false) override;
    CORE_API IFileHandle* OpenWrite(const TCHAR* filename, bool append = false, bool allowRead = false) override;
    CORE_API bool DirectoryExists(const TCHAR* directory) override;
    CORE_API bool CreateDirectory(const TCHAR* directory) override;
    CORE_API bool DeleteDirectory(const TCHAR* directory) override;
    CORE_API FFileStatData GetStatData(const TCHAR* filename) override;
    CORE_API bool IterateDirectory(const TCHAR* directory, FDirectoryVisitor& visitor) override;
    CORE_API bool IterateDirectoryStat(const TCHAR* directory, FDirectoryStatVisitor& visitor) override;
};

#endif
