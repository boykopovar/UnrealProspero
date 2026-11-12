#ifndef UNREALPROSPERO_SAVE_DATA_MOUNT_H
#define UNREALPROSPERO_SAVE_DATA_MOUNT_H

#include "UnrealProspero/SaveData/TransactionResource.h"
#include "prx/libSceSaveData.native/libSceSaveData.native.hpp"
#include <string_view>

namespace UnrealProspero::SaveData
{
    class Mount final
    {
    public:
        Mount(int userId, std::string_view directory, std::uint64_t blocks, std::uint32_t resourceSize);
        ~Mount() noexcept(false);
        Mount(const Mount&) = delete;
        Mount& operator=(const Mount&) = delete;
        std::string_view GetPath() const;
        bool IsNew() const;
        void Prepare();
        void SetParameters(const SaveDataParam& parameters);
        void Commit();
        void Cancel();
        void Close();

    private:
        TransactionResource resource;
        SaveDataMountResult result{};
        bool mounted = false;
        bool prepared = false;
        void requireMounted() const;
        void unmount(std::uint32_t mode);
    };
}

#endif
