#include "AnyUnreal/Kernel/FileLock.h"
#include "AnyUnreal/Kernel/File.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include "AnyUnreal/Kernel/Clock.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <cstring>
#include <string>

namespace
{
    using namespace AnyUnreal::Kernel;

    class LockPath
    {
    public:
        explicit LockPath(std::string path) : path(std::move(path))
        {
            File file(this->path, FileAccess::Write, FileCreation::CreateNew);
        }

        ~LockPath() noexcept(false) { FileSystem::RemoveFile(path); }

    private:
        std::string path;
    };
}

bool CheckFileLocks()
{
    const auto path = "AnyUnreal-lock-" + std::to_string(getpid_nid_postfix());
    LockPath cleanup(path);
    {
        FileLock owner(path);
        if (!owner.IsOwned()) return false;
        FileLock immediate(path);
        if (immediate.IsOwned()) return false;
        const auto start = Clock::MonotonicNanoseconds();
        FileLock timed(path, 20000000);
        if (timed.IsOwned() || Clock::MonotonicNanoseconds() - start < 20000000) return false;
        owner.Release();
        if (owner.IsOwned()) return false;
        owner.Release();
        FileLock next(path);
        if (!next.IsOwned()) return false;
    }
    FileLock afterDestruction(path);
    return afterDestruction.IsOwned();
}

int RunFileLockProbe(int count, const char** arguments)
{
    if (count == 3 && std::strcmp(arguments[1], "--try-file-lock") == 0)
    {
        FileLock lock(arguments[2]);
        return lock.IsOwned() ? 0 : 41;
    }
    if (count == 4 && std::strcmp(arguments[1], "--hold-file-lock") == 0)
    {
        FileLock lock(arguments[2]);
        if (!lock.IsOwned()) return 42;
        {
            File ready(arguments[3], FileAccess::Write, FileCreation::CreateNew);
        }
        for (;;)
        {
            const int result = sceKernelUsleep_nid_postfix(1000000);
            if (result != SCE_KERNEL_ERROR_EINTR) CheckResult(result, "sceKernelUsleep");
        }
    }
    throw std::invalid_argument("Invalid file lock probe arguments");
}
