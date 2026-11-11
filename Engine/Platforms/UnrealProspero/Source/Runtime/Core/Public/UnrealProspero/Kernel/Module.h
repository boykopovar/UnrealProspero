#ifndef UNREALPROSPERO_KERNEL_MODULE_H
#define UNREALPROSPERO_KERNEL_MODULE_H

#include "SceTypes.hpp"
#include <optional>
#include <string_view>

namespace UnrealProspero::Kernel
{
    class Module final
    {
    public:
        explicit Module(std::string_view path);
        ~Module() noexcept(false);
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
        void* FindSymbol(std::string_view name) const;
        void Close();

    private:
        std::optional<KernelModule> handle;
    };
}

#endif
