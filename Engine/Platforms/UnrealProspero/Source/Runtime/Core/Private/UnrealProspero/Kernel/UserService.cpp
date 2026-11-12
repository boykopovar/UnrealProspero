#include "UnrealProspero/Kernel/UserService.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Mutex/ScopedLock.h"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libSceSysmodule/include/Api.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>
#include <iterator>
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
            std::vector<int> Users;
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

    int UserService::GetUserIndex(int userId) const
    {
        if (!active) throw std::logic_error("User service is closed");
        if (userId < 0) throw std::invalid_argument("Invalid user ID");
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.Users.empty()) service.Users.push_back(GetInitialUser());
        const auto found = std::find(service.Users.begin(), service.Users.end(), userId);
        if (found != service.Users.end()) return static_cast<int>(found - service.Users.begin());
        UserServiceLoginUserIdList users{};
        CheckResult(sceUserServiceGetLoginUserIdList(&users), "sceUserServiceGetLoginUserIdList");
        if (std::find(std::begin(users.user_id), std::end(users.user_id), userId) == std::end(users.user_id)) throw std::invalid_argument("User is not logged in");
        if (service.Users.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::overflow_error("User index overflow");
        const int index = static_cast<int>(service.Users.size());
        service.Users.push_back(userId);
        return index;
    }

    int UserService::GetUserId(int userIndex) const
    {
        if (!active) throw std::logic_error("User service is closed");
        if (userIndex < 0) throw std::invalid_argument("Invalid user index");
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.Users.empty()) service.Users.push_back(GetInitialUser());
        if (static_cast<std::size_t>(userIndex) >= service.Users.size()) throw std::out_of_range("Unknown user index");
        return service.Users[static_cast<std::size_t>(userIndex)];
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
