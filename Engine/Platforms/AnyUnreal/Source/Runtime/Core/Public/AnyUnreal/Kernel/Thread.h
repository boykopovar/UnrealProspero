#ifndef ANYUNREAL_KERNEL_THREAD_H
#define ANYUNREAL_KERNEL_THREAD_H

#include "AnyUnreal/Kernel/ThreadAttributes.h"
#include <exception>
#include <functional>
#include <string_view>

namespace AnyUnreal::Kernel
{
    class Thread final
    {
    public:
        Thread(std::function<void()> function, std::string_view name, const ThreadAttributes* attributes = nullptr);
        ~Thread() noexcept(false);
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
        void Join();
        bool IsJoinable() const noexcept;
        Pthread Get() const;

    private:
        static void* APS5_VABI run(void* argument);
        std::function<void()> function;
        std::exception_ptr failure;
        Pthread handle = nullptr;
    };
}

#endif
