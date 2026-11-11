#ifndef UNREALPROSPERO_KERNEL_THREAD_ATTRIBUTES_H
#define UNREALPROSPERO_KERNEL_THREAD_ATTRIBUTES_H

#include "SceTypes.hpp"
#include <cstddef>

namespace UnrealProspero::Kernel
{
    class ThreadAttributes final
    {
    public:
        ThreadAttributes();
        ~ThreadAttributes() noexcept(false);
        ThreadAttributes(const ThreadAttributes&) = delete;
        ThreadAttributes& operator=(const ThreadAttributes&) = delete;
        void SetStackSize(std::size_t size);
        void SetPriority(int priority);
        void SetExclusiveScheduling(bool exclusive);
        void SetAffinity(KernelCpumask mask);
        const PthreadAttr* Get() const noexcept;

    private:
        PthreadAttr handle = nullptr;
    };
}

#endif
