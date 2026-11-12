#ifndef UNREALPROSPERO_SAVE_DATA_TRANSACTION_RESOURCE_H
#define UNREALPROSPERO_SAVE_DATA_TRANSACTION_RESOURCE_H

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
        int handle = -1;
        bool active = false;
    };
}

#endif
