#include "UnrealProspero/SaveData/Catalog.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceSaveData.native/libSceSaveData.native.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace UnrealProspero::SaveData
{
    namespace
    {
        SceSaveDataDirName encodeName(std::string_view directory)
        {
            SceSaveDataDirName name{};
            if (directory.empty() || directory.size() >= sizeof(name.data) || directory.find_first_of("/\\*?") != std::string_view::npos || directory.find('\0') != std::string_view::npos || directory == "." || directory == "..") throw std::invalid_argument("Invalid SaveData directory name");
            std::copy(directory.begin(), directory.end(), name.data);
            return name;
        }
    }

    Catalog::Catalog(std::uint32_t resourceSize) : resource(resourceSize) {}

    std::vector<std::string> Catalog::List(int userId) const
    {
        if (userId < 0) throw std::invalid_argument("Invalid SaveData user");
        SaveDataDirNameSearchCond condition{};
        condition.user_id = userId;
        std::vector<SceSaveDataDirName> names(1);
        SaveDataDirNameSearchResult result{};
        result.dir_names = names.data();
        result.dir_names_num = 1;
        Kernel::CheckResult(sceSaveDataDirNameSearch(&condition, &result), "sceSaveDataDirNameSearch");
        if (result.set_num != std::min(result.hit_num, 1u)) throw std::runtime_error("Invalid SaveData count query result");
        if (result.hit_num == 0) return {};
        if (result.hit_num > 1024) throw std::runtime_error("SaveData directory count exceeds search capacity");
        if (result.hit_num > names.size())
        {
            names.resize(result.hit_num);
            result = {};
            result.dir_names = names.data();
            result.dir_names_num = static_cast<std::uint32_t>(names.size());
            Kernel::CheckResult(sceSaveDataDirNameSearch(&condition, &result), "sceSaveDataDirNameSearch");
            if (result.hit_num > names.size() || result.set_num != result.hit_num) throw std::runtime_error("Incomplete SaveData directory search");
        }
        std::vector<std::string> directories;
        directories.reserve(result.set_num);
        for (std::size_t index = 0; index < result.set_num; ++index)
        {
            const auto& name = names[index].data;
            const auto end = std::find(std::begin(name), std::end(name), '\0');
            if (end == std::begin(name) || end == std::end(name)) throw std::runtime_error("Invalid SaveData directory result");
            directories.emplace_back(std::begin(name), end);
        }
        std::sort(directories.begin(), directories.end());
        if (std::adjacent_find(directories.begin(), directories.end()) != directories.end()) throw std::runtime_error("Duplicate SaveData directory result");
        return directories;
    }

    bool Catalog::Exists(int userId, std::string_view directory) const
    {
        encodeName(directory);
        const auto directories = List(userId);
        return std::binary_search(directories.begin(), directories.end(), directory);
    }

    void Catalog::Delete(int userId, std::string_view directory) const
    {
        if (userId < 0) throw std::invalid_argument("Invalid SaveData user");
        const auto name = encodeName(directory);
        SaveDataDelete request{};
        request.user_id = userId;
        request.dir_name = &name;
        Kernel::CheckResult(sceSaveDataDelete(&request), "sceSaveDataDelete");
    }
}
