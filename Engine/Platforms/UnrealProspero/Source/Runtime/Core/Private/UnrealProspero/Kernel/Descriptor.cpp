#include "UnrealProspero/Kernel/Descriptor.h"
#include "UnrealProspero/Kernel/Path.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/File/include/File.hpp"
#include <utility>

namespace UnrealProspero::Kernel
{
    Descriptor Descriptor::Open(std::string_view path, int flags, std::uint16_t permissions)
    {
        const int handle = sceKernelOpen(Path(path).CStr(), flags, permissions);
        if (handle < 0) throw Error(handle, "sceKernelOpen");
        return Descriptor(handle);
    }

    Descriptor::~Descriptor() noexcept(false)
    {
        if (value != -1) Close();
    }

    int Descriptor::Get() const
    {
        if (value < 0) throw std::logic_error("File descriptor is closed");
        return value;
    }

    void Descriptor::Close()
    {
        Get();
        CheckResult(sceKernelClose(std::exchange(value, -1)), "sceKernelClose");
    }
}
