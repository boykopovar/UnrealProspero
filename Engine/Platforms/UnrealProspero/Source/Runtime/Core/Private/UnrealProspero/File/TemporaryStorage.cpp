#include "UnrealProspero/File/TemporaryStorage.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceSysmodule/include/Api.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace UnrealProspero::File
{
    namespace
    {
        constexpr unsigned short moduleId = 0xb4;
    }

    TemporaryStorage::TemporaryStorage()
    {
        Kernel::CheckResult(sceSysmoduleLoadModule(moduleId), "sceSysmoduleLoadModule");
        loaded = true;
        try
        {
            AppContentInitParam init{};
            AppContentBootParam boot{};
            Kernel::CheckResult(sceAppContentInitialize(&init, &boot), "sceAppContentInitialize");
            Kernel::CheckResult(sceAppContentTemporaryDataMount2(0, &mountPoint), "sceAppContentTemporaryDataMount2");
            mounted = true;
            GetPath();
        }
        catch (...)
        {
            Close();
            throw;
        }
    }

    TemporaryStorage::~TemporaryStorage() noexcept(false) { Close(); }

    std::string_view TemporaryStorage::GetPath() const
    {
        if (!mounted) throw std::logic_error("Temporary storage is not mounted");
        const auto end = std::find(std::begin(mountPoint.data), std::end(mountPoint.data), '\0');
        if (end == std::end(mountPoint.data) || mountPoint.data[0] != '/' || end == std::begin(mountPoint.data) + 1) throw std::runtime_error("Invalid temporary storage mount point");
        return {mountPoint.data, static_cast<std::size_t>(end - mountPoint.data)};
    }

    void TemporaryStorage::Close()
    {
        if (mounted)
        {
            Kernel::CheckResult(sceAppContentTemporaryDataUnmount(&mountPoint), "sceAppContentTemporaryDataUnmount");
            mounted = false;
        }
        if (loaded)
        {
            Kernel::CheckResult(sceSysmoduleUnloadModule(moduleId), "sceSysmoduleUnloadModule");
            loaded = false;
        }
    }
}
