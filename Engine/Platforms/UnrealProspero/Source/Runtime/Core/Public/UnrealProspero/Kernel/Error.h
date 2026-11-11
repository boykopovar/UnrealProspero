#ifndef UNREALPROSPERO_KERNEL_ERROR_H
#define UNREALPROSPERO_KERNEL_ERROR_H

#include <cstdint>
#include <stdexcept>
#include <string>

namespace UnrealProspero::Kernel
{
    class Error final : public std::runtime_error
    {
    public:
        Error(std::int64_t code, const char* operation) : std::runtime_error(std::string(operation) + ": " + std::to_string(code)), code(code) {}
        std::int64_t Code() const noexcept { return code; }

    private:
        std::int64_t code;
    };
}

#endif
