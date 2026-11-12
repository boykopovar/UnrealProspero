#ifndef UNREALPROSPERO_SAVE_DATA_TRANSACTION_RESOURCE_H
#define UNREALPROSPERO_SAVE_DATA_TRANSACTION_RESOURCE_H

#include "UnrealProspero/Kernel/UserService.h"
#include <cstdint>

namespace UnrealProspero::SaveData
{
    class TransactionResource final
    {
    public:
        explicit TransactionResource(std::uint32_t size);
        ~TransactionResource() noexcept(false);
        TransactionResource(const TransactionResource&) = delete;
        TransactionResource& operator=(const TransactionResource&) = delete;
        int GetHandle() const;
        void Close();

    private:
        Kernel::UserService userService;
        int handle = -1;
        bool active = false;
    };
}

#endif
