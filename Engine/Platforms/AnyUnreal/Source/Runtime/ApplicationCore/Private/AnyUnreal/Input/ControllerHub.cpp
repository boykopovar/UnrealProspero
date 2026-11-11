#include "AnyUnreal/Input/ControllerHub.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include "prx/libSceSysmodule/include/Api.hpp"
#include <set>
#include <stdexcept>

namespace AnyUnreal::Input
{
    ControllerHub::ControllerHub()
    {
        Kernel::CheckResult(sceSysmoduleLoadModuleInternal(0x80000011), "sceSysmoduleLoadModuleInternal");
        Kernel::CheckResult(sceSysmoduleLoadModuleInternal(0x80000024), "sceSysmoduleLoadModuleInternal");
        Kernel::CheckResult(sceUserServiceInitialize(nullptr), "sceUserServiceInitialize");
        initialized = true;
        try
        {
            Kernel::CheckResult(sceUserServiceGetInitialUser(&initialUser), "sceUserServiceGetInitialUser");
            if (initialUser < 0) throw std::runtime_error("User service returned an invalid initial user");
            Kernel::CheckResult(scePadInit_nid_postfix(), "scePadInit");
        }
        catch (...)
        {
            Close();
            throw;
        }
    }

    ControllerHub::~ControllerHub() noexcept(false) { Close(); }

    std::vector<ControllerSample> ControllerHub::Poll()
    {
        if (!initialized) throw std::runtime_error("Controller service is closed");
        UserServiceLoginUserIdList users{};
        Kernel::CheckResult(sceUserServiceGetLoginUserIdList(&users), "sceUserServiceGetLoginUserIdList");
        std::set<int> active;
        for (const auto user : users.user_id)
        {
            if (user == USER_SERVICE_USER_ID_INVALID) continue;
            if (user < 0 || !active.insert(user).second) throw std::runtime_error("User service returned an invalid login list");
        }
        for (auto iterator = handles.begin(); iterator != handles.end();)
        {
            if (active.contains(iterator->first)) ++iterator;
            else
            {
                if (iterator->second) Kernel::CheckResult(scePadClose_nid_postfix(*iterator->second), "scePadClose");
                iterator = handles.erase(iterator);
            }
        }
        std::vector<ControllerSample> result;
        result.reserve(active.size());
        for (const auto user : users.user_id)
        {
            if (user == USER_SERVICE_USER_ID_INVALID) continue;
            auto& handle = handles.try_emplace(user, std::nullopt).first->second;
            if (!handle)
            {
                const auto opened = scePadOpen_nid_postfix(user, PAD_PORT_TYPE_STANDARD, 0, nullptr);
                if (opened < 0) throw Kernel::Error(opened, "scePadOpen");
                handle = opened;
            }
            ControllerSample sample{user, {}};
            Kernel::CheckResult(scePadReadState(*handle, &sample.Data), "scePadReadState");
            result.push_back(sample);
        }
        return result;
    }

    int ControllerHub::GetInitialUser() const
    {
        if (!initialized) throw std::runtime_error("Controller service is closed");
        return initialUser;
    }

    void ControllerHub::Close()
    {
        if (!initialized) return;
        for (auto& [user, handle] : handles)
        {
            if (handle)
            {
                Kernel::CheckResult(scePadClose_nid_postfix(*handle), "scePadClose");
                handle.reset();
            }
        }
        handles.clear();
        Kernel::CheckResult(sceUserServiceTerminate(), "sceUserServiceTerminate");
        initialized = false;
    }
}
