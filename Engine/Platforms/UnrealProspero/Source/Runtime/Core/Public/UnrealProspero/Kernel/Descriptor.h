#ifndef UNREALPROSPERO_KERNEL_DESCRIPTOR_H
#define UNREALPROSPERO_KERNEL_DESCRIPTOR_H

#include <cstdint>
#include <string_view>

namespace UnrealProspero::Kernel
{
    class Descriptor final
    {
    public:
        static Descriptor Open(std::string_view path, int flags, std::uint16_t permissions = 0);
        ~Descriptor() noexcept(false);
        Descriptor(const Descriptor&) = delete;
        Descriptor& operator=(const Descriptor&) = delete;
        int Get() const;
        void Close();

    private:
        explicit Descriptor(int value) noexcept : value(value) {}
        int value;
    };
}

#endif
