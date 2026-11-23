#ifndef UNREALPROSPERO_RHI_MEMORY_GPU_ADDRESS_RESERVATION_H
#define UNREALPROSPERO_RHI_MEMORY_GPU_ADDRESS_RESERVATION_H

#include <cstddef>

namespace UnrealProspero::RHI
{
    class GpuAddressReservation final
    {
    public:
        GpuAddressReservation(void* address, std::size_t size);
        ~GpuAddressReservation() noexcept(false);
        GpuAddressReservation(const GpuAddressReservation&) = delete;
        GpuAddressReservation& operator=(const GpuAddressReservation&) = delete;

    private:
        void* address;
        std::size_t size;
    };
}

#endif
