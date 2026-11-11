#include "UnrealProspero/Kernel/FileSystem.h"
#include "UnrealProspero/Kernel/Path.h"
#include "UnrealProspero/Kernel/PosixResult.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel_sys/include/Api.hpp"
#include "prx/libkernel/File/include/File.hpp"

namespace UnrealProspero::Kernel
{
    bool FileSystem::MoveNoReplace(std::string_view source, std::string_view destination)
    {
        const Path sourcePath(source);
        const Path destinationPath(destination);
        const int result = linkat_nid_postfix(-100, sourcePath.CStr(), -100, destinationPath.CStr(), 0);
        if (result == -1 && errno == EEXIST) return false;
        CheckPosixResult(result, "linkat");
        CheckResult(sceKernelUnlink(sourcePath.CStr()), "unlink after linkat; destination retained on failure");
        return true;
    }
}
