#include "UnrealProspero/SaveData/TransactionResource.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Mutex/ScopedLock.h"
#include "prx/libSceSaveData.native/libSceSaveData.native.hpp"
#include "prx/libSceSysmodule/include/Api.hpp"
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::SaveData
{
    namespace
    {
        constexpr std::uint32_t moduleId = 0x8000000f;

        struct State
        {
            UE::FUnrealProsperoRecursiveMutex Mutex;
            std::size_t References = 0;
            bool Loaded = false;
            bool Initialized = false;
        };

        State& state()
        {
            static State value;
            return value;
        }

        void releaseService(State& service)
        {
            if (service.Initialized)
            {
                Kernel::CheckResult(sceSaveDataTerminate(), "sceSaveDataTerminate");
                service.Initialized = false;
            }
            if (service.Loaded)
            {
                Kernel::CheckResult(sceSysmoduleUnloadModuleInternal(moduleId), "sceSysmoduleUnloadModuleInternal");
                service.Loaded = false;
            }
        }
    }

    TransactionResource::TransactionResource(std::uint32_t size)
    {
        if (size == 0) throw std::invalid_argument("SaveData transaction resource size is zero");
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.References == std::numeric_limits<std::size_t>::max()) throw std::overflow_error("SaveData reference count overflow");
        try
        {
            if (!service.Loaded)
            {
                Kernel::CheckResult(sceSysmoduleLoadModuleInternal(moduleId), "sceSysmoduleLoadModuleInternal");
                service.Loaded = true;
            }
            if (!service.Initialized)
            {
                Kernel::CheckResult(sceSaveDataInitialize3(nullptr), "sceSaveDataInitialize3");
                service.Initialized = true;
            }
            const int result = sceSaveDataCreateTransactionResource(size);
            if (result < 0) Kernel::CheckResult(result, "sceSaveDataCreateTransactionResource");
            handle = result;
        }
        catch (...)
        {
            if (service.References == 0) releaseService(service);
            throw;
        }
        ++service.References;
        active = true;
    }

    TransactionResource::~TransactionResource() noexcept(false) { Close(); }

    int TransactionResource::GetHandle() const
    {
        if (!active || handle < 0) throw std::logic_error("SaveData transaction resource is closed");
        return handle;
    }

    void TransactionResource::Close()
    {
        if (!active) return;
        auto& service = state();
        ScopedLock lock(service.Mutex);
        if (service.References == 0) throw std::logic_error("Invalid SaveData reference count");
        if (handle >= 0)
        {
            Kernel::CheckResult(sceSaveDataDeleteTransactionResource(handle), "sceSaveDataDeleteTransactionResource");
            handle = -1;
        }
        if (service.References == 1) releaseService(service);
        --service.References;
        active = false;
    }
}
