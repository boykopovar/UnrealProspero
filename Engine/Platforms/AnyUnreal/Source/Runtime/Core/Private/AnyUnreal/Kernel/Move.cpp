#include "AnyUnreal/Kernel/FileSystem.h"
#include "AnyUnreal/Kernel/Path.h"
#include "AnyUnreal/Kernel/PosixResult.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel_sys/include/Api.hpp"
#include "prx/libkernel/File/include/File.hpp"

namespace AnyUnreal::Kernel
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
