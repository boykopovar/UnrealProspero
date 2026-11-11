#include "UnrealProspero/Kernel/Thread.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <string>
#include <utility>

namespace UnrealProspero::Kernel
{
    Thread::Thread(std::function<void()> function, std::string_view name, const ThreadAttributes* attributes) : function(std::move(function))
    {
        if (!this->function) throw std::invalid_argument("Thread function is empty");
        if (name.find('\0') != std::string_view::npos) throw std::invalid_argument("Thread name contains a null character");
        const std::string terminatedName(name);
        CheckResult(scePthreadCreate(&handle, attributes ? attributes->Get() : nullptr, run, this, terminatedName.c_str()), "scePthreadCreate");
    }

    Thread::~Thread() noexcept(false)
    {
        Join();
    }

    void Thread::Join()
    {
        if (!handle) return;
        CheckResult(scePthreadJoin(handle, nullptr), "scePthreadJoin");
        handle = nullptr;
        if (failure) std::rethrow_exception(std::exchange(failure, {}));
    }

    bool Thread::IsJoinable() const noexcept
    {
        return handle != nullptr;
    }

    Pthread Thread::Get() const
    {
        if (!handle) throw std::logic_error("Thread has already been joined");
        return handle;
    }

    void* APS5_VABI Thread::run(void* argument)
    {
        auto& thread = *static_cast<Thread*>(argument);
        try
        {
            thread.function();
        }
        catch (...)
        {
            thread.failure = std::current_exception();
        }
        return nullptr;
    }
}
