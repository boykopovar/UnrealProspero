#include "UnrealProspero/SaveGameSystem.h"
#include "UnrealProspero/Kernel/UserService.h"
#include "UnrealProspero/Mutex/ScopedLock.h"
#include "UnrealProspero/SaveData/Catalog.h"
#include "UnrealProspero/SaveData/Mount.h"
#include "UnrealProspero/SaveData/SlotFile.h"
#include "UnrealProspero/Text/Encoding.h"
#include "Misc/Base64.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/SecureHash.h"
#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <string>

namespace UnrealProspero
{
    namespace
    {
        constexpr std::uint32_t resourceSize = 786432;

        std::string slotName(const TCHAR* name)
        {
            if (!name || !*name) throw std::invalid_argument("SaveGame slot name is empty");
            return Text::EncodeUtf8(name);
        }

        std::string directoryName(std::string_view name)
        {
            std::array<uint8, 20> hash{};
            FSHA1::HashBuffer(name.data(), name.size(), hash.data());
            FString encoded = FBase64::Encode(hash.data(), hash.size(), EBase64Mode::UrlSafe);
            encoded.RemoveFromEnd(TEXT("="));
            return "ue_" + Text::EncodeUtf8(*encoded);
        }

        std::string filePath(const SaveData::Mount& mount)
        {
            return std::string(mount.GetPath()) + "/slot.bin";
        }

        void verifyName(const SaveData::Mount& mount, std::string_view expected)
        {
            if (SaveData::SlotFile::ReadName(filePath(mount)) != expected) throw std::runtime_error("SaveGame directory name collision");
        }
    }

    SaveGameSystem::SaveGameSystem()
    {
        if (!GConfig) throw std::runtime_error("SaveGame configuration is unavailable");
        int32 configured = 1024;
        GConfig->GetInt(TEXT("UnrealProspero.SaveGame"), TEXT("BlocksPerSlot"), configured, GEngineIni);
        if (configured < 48 || configured > 16384) throw std::invalid_argument("SaveGame BlocksPerSlot must be between 48 and 16384");
        blocks = static_cast<std::uint64_t>(configured);
    }

    bool SaveGameSystem::PlatformHasNativeUI() { return false; }
    bool SaveGameSystem::DoesSaveSystemSupportMultipleUsers() { return true; }

    bool SaveGameSystem::DoesSaveGameExist(const TCHAR* name, int32 userIndex)
    {
        return DoesSaveGameExistWithResult(name, userIndex) == ESaveExistsResult::OK;
    }

    ISaveGameSystem::ESaveExistsResult SaveGameSystem::DoesSaveGameExistWithResult(const TCHAR* name, int32 userIndex)
    {
        ScopedLock lock(mutex);
        Kernel::UserService users;
        const int user = users.GetUserId(userIndex);
        const auto slot = slotName(name);
        const auto directory = directoryName(slot);
        SaveData::Catalog catalog(resourceSize);
        if (!catalog.Exists(user, directory)) return ESaveExistsResult::DoesNotExist;
        SaveData::Mount mount(user, directory, 0, resourceSize, SaveData::Mount::Access::ReadOnly);
        verifyName(mount, slot);
        mount.Close();
        return ESaveExistsResult::OK;
    }

    bool SaveGameSystem::GetSaveGameNames(TArray<FString>& names, int32 userIndex)
    {
        ScopedLock lock(mutex);
        Kernel::UserService users;
        const int user = users.GetUserId(userIndex);
        SaveData::Catalog catalog(resourceSize);
        TArray<FString> found;
        for (const auto& directory : catalog.List(user))
        {
            if (!directory.starts_with("ue_")) continue;
            SaveData::Mount mount(user, directory, 0, resourceSize, SaveData::Mount::Access::ReadOnly);
            const auto name = SaveData::SlotFile::ReadName(filePath(mount));
            if (directoryName(name) != directory) throw std::runtime_error("SaveGame directory does not match its slot name");
            found.Emplace(Text::DecodeUtf8(name).c_str());
            mount.Close();
        }
        found.Sort();
        names = MoveTemp(found);
        return true;
    }

    bool SaveGameSystem::SaveGame(bool, const TCHAR* name, int32 userIndex, const TArray<uint8>& data)
    {
        ScopedLock lock(mutex);
        Kernel::UserService users;
        const auto slot = slotName(name);
        SaveData::Mount mount(users.GetUserId(userIndex), directoryName(slot), blocks, resourceSize);
        if (!mount.IsNew()) verifyName(mount, slot);
        mount.Prepare();
        if (mount.IsNew())
        {
            SaveDataParam parameters{};
            constexpr char title[] = "Unreal SaveGame";
            std::copy(std::begin(title), std::end(title), parameters.title);
            mount.SetParameters(parameters);
        }
        SaveData::SlotFile::Write(filePath(mount), slot, std::as_bytes(std::span(data.GetData(), static_cast<std::size_t>(data.Num()))));
        mount.Commit();
        mount.Close();
        return true;
    }

    bool SaveGameSystem::LoadGame(bool, const TCHAR* name, int32 userIndex, TArray<uint8>& data)
    {
        ScopedLock lock(mutex);
        Kernel::UserService users;
        const int user = users.GetUserId(userIndex);
        const auto slot = slotName(name);
        const auto directory = directoryName(slot);
        SaveData::Catalog catalog(resourceSize);
        if (!catalog.Exists(user, directory)) return false;
        SaveData::Mount mount(user, directory, 0, resourceSize, SaveData::Mount::Access::ReadOnly);
        const auto bytes = SaveData::SlotFile::Read(filePath(mount), slot);
        TArray<uint8> loaded;
        loaded.Append(reinterpret_cast<const uint8*>(bytes.data()), static_cast<int32>(bytes.size()));
        mount.Close();
        data = MoveTemp(loaded);
        return true;
    }

    bool SaveGameSystem::DeleteGame(bool, const TCHAR* name, int32 userIndex)
    {
        ScopedLock lock(mutex);
        Kernel::UserService users;
        const int user = users.GetUserId(userIndex);
        const auto slot = slotName(name);
        const auto directory = directoryName(slot);
        SaveData::Catalog catalog(resourceSize);
        if (!catalog.Exists(user, directory)) return false;
        SaveData::Mount mount(user, directory, 0, resourceSize, SaveData::Mount::Access::ReadOnly);
        verifyName(mount, slot);
        mount.Close();
        catalog.Delete(user, directory);
        return true;
    }
}
