#ifndef ANYUNREAL_KERNEL_THREAD_ATTRIBUTES_H
#define ANYUNREAL_KERNEL_THREAD_ATTRIBUTES_H

#include "SceTypes.hpp"
#include <cstddef>

namespace AnyUnreal::Kernel
{
    class ThreadAttributes final
    {
    public:
        ThreadAttributes();
        ~ThreadAttributes() noexcept(false);
        ThreadAttributes(const ThreadAttributes&) = delete;
        ThreadAttributes& operator=(const ThreadAttributes&) = delete;
        void SetStackSize(std::size_t size);
        void SetAffinity(KernelCpumask mask);
        const PthreadAttr* Get() const noexcept;

    private:
        PthreadAttr handle = nullptr;
    };
}

#endif
