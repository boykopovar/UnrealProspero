#ifndef ANYUNREAL_KERNEL_PATH_H
#define ANYUNREAL_KERNEL_PATH_H

#include <stdexcept>
#include <string>
#include <string_view>

namespace AnyUnreal::Kernel
{
    class Path final
    {
    public:
        explicit Path(std::string_view path) : value(path)
        {
            if (value.empty() || value.find('\0') != std::string::npos) throw std::invalid_argument("Invalid file path");
        }

        const char* CStr() const noexcept { return value.c_str(); }

    private:
        std::string value;
    };
}

#endif
