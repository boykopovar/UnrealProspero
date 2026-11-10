#include "AnyUnreal/Kernel/Module.h"
#include "AnyUnreal/Kernel/Path.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <string>

namespace AnyUnreal::Kernel
{
    Module::Module(std::string_view path)
    {
        const Path terminatedPath(path);
        int result = 0;
        const KernelModule loaded = sceKernelLoadStartModule(terminatedPath.CStr(), 0, nullptr, 0, nullptr, &result);
        if (loaded < 0) throw Error(loaded, "sceKernelLoadStartModule");
        handle = loaded;
        if (result != 0)
        {
            Close();
            throw Error(result, "module_start");
        }
    }

    Module::~Module() noexcept(false)
    {
        Close();
    }

    void* Module::FindSymbol(std::string_view name) const
    {
        if (!handle) throw std::logic_error("Module is closed");
        if (name.empty() || name.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid module symbol name");
        const std::string terminatedName(name);
        void* address = nullptr;
        CheckResult(sceKernelDlsym(*handle, terminatedName.c_str(), &address), "sceKernelDlsym");
        if (address == nullptr) throw std::runtime_error("Module symbol has a null address");
        return address;
    }

    void Module::Close()
    {
        if (!handle) return;
        int result = 0;
        CheckResult(sceKernelStopUnloadModule(*handle, 0, nullptr, 0, nullptr, &result), "sceKernelStopUnloadModule");
        handle.reset();
        CheckResult(result, "module_stop");
    }
}
