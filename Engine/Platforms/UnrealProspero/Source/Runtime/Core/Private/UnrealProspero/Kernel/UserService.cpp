#include "UnrealProspero/Kernel/UserService.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Mutex/ScopedLock.h"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libSceSysmodule/include/Api.hpp"
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Kernel
{
    namespace
    {
        constexpr std::uint32_t moduleId = 0x80000011;

        struct State
        {
            UE::FUnrealProsperoRecursiveMutex Mutex;
            std::size_t References = 0;
        };

        State& state()
        {
            static State value;
            return value;
        }
    }

    UserService::UserService()
    {
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.References == std::numeric_limits<std::size_t>::max()) throw std::overflow_error("User service reference count overflow");
        if (service.References == 0)
        {
            CheckResult(sceSysmoduleLoadModuleInternal(moduleId), "sceSysmoduleLoadModuleInternal");
            try
            {
                CheckResult(sceUserServiceInitialize(nullptr), "sceUserServiceInitialize");
            }
            catch (...)
            {
                CheckResult(sceSysmoduleUnloadModuleInternal(moduleId), "sceSysmoduleUnloadModuleInternal");
                throw;
            }
        }
        ++service.References;
        active = true;
    }

    UserService::~UserService() noexcept(false) { Close(); }

    int UserService::GetInitialUser() const
    {
        if (!active) throw std::logic_error("User service is closed");
        int user = -1;
        CheckResult(sceUserServiceGetInitialUser(&user), "sceUserServiceGetInitialUser");
        if (user < 0) throw std::runtime_error("User service returned an invalid initial user");
        return user;
    }

    void UserService::Close()
    {
        if (!active) return;
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.References == 0) throw std::logic_error("Invalid user service reference count");
        if (service.References == 1)
        {
            CheckResult(sceUserServiceTerminate(), "sceUserServiceTerminate");
            CheckResult(sceSysmoduleUnloadModuleInternal(moduleId), "sceSysmoduleUnloadModuleInternal");
        }
        --service.References;
        active = false;
    }
}
