#include "UnrealProspero/SaveData/Mount.h"
#include "UnrealProspero/Kernel/Result.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::SaveData
{
    Mount::Mount(int userId, std::string_view directory, std::uint64_t blocks, std::uint32_t resourceSize, Access access) : resource(resourceSize), access(access)
    {
        SceSaveDataDirName name{};
        if (directory.empty() || directory.size() >= sizeof(name.data) || directory.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid SaveData directory name");
        std::copy(directory.begin(), directory.end(), name.data);
        SaveDataMount3 request{};
        request.user_id = userId;
        request.dir_name = &name;
        switch (access)
        {
        case Access::ReadOnly: request.mount_mode = 1; break;
        case Access::ReadWrite: request.mount_mode = 1; break;
        case Access::CreateOrOpen:
            request.blocks = blocks;
            request.system_blocks = std::numeric_limits<std::uint64_t>::max();
            request.mount_mode = 32;
            break;
        default: throw std::invalid_argument("Invalid SaveData mount access");
        }
        request.resource = resource.GetHandle();
        Kernel::CheckResult(sceSaveDataMount3(&request, &result), "sceSaveDataMount3");
        mounted = true;
        try
        {
            GetPath();
        }
        catch (...)
        {
            Close();
            throw;
        }
    }

    Mount::~Mount() noexcept(false) { Cancel(); }

    void Mount::requireMounted() const
    {
        if (!mounted) throw std::logic_error("SaveData is not mounted");
    }

    std::string_view Mount::GetPath() const
    {
        requireMounted();
        const auto& path = result.mount_point.data;
        const auto end = std::find(std::begin(path), std::end(path), '\0');
        if (end == std::end(path) || path[0] != '/' || end == std::begin(path) + 1) throw std::runtime_error("Invalid SaveData mount point");
        return {path, static_cast<std::size_t>(end - path)};
    }

    Mount::Space Mount::GetSpace() const
    {
        requireMounted();
        SaveDataMountInfo info{};
        Kernel::CheckResult(sceSaveDataGetMountInfo(&result.mount_point, &info), "sceSaveDataGetMountInfo");
        if (info.blocks == 0 || info.free_blocks > info.blocks) throw std::runtime_error("Invalid SaveData capacity");
        return {info.blocks, info.free_blocks};
    }

    bool Mount::IsNew() const
    {
        requireMounted();
        return (result.mount_status & 1u) != 0;
    }

    void Mount::Prepare()
    {
        requireMounted();
        if (access == Access::ReadOnly) throw std::logic_error("SaveData mount is read-only");
        if (prepared) throw std::logic_error("SaveData transaction is already prepared");
        SaveDataPrepareParam parameters{};
        parameters.resource = resource.GetHandle();
        parameters.prepare_mode = 2;
        Kernel::CheckResult(sceSaveDataPrepare(&result.mount_point, &parameters), "sceSaveDataPrepare");
        prepared = true;
    }

    void Mount::SetParameters(const SaveDataParam& parameters)
    {
        requireMounted();
        if (!prepared) throw std::logic_error("SaveData transaction is not prepared");
        Kernel::CheckResult(sceSaveDataSetParam(&result.mount_point, 0, &parameters, sizeof(parameters)), "sceSaveDataSetParam");
    }

    void Mount::Commit()
    {
        requireMounted();
        if (!prepared) throw std::logic_error("SaveData transaction is not prepared");
        SaveDataCommitParam parameters{};
        parameters.resource = resource.GetHandle();
        Kernel::CheckResult(sceSaveDataCommit(&parameters), "sceSaveDataCommit");
        prepared = false;
    }

    void Mount::unmount(std::uint32_t mode)
    {
        if (mounted)
        {
            Kernel::CheckResult(sceSaveDataUmount2(mode, &result.mount_point), "sceSaveDataUmount2");
            mounted = false;
            prepared = false;
        }
        resource.Close();
    }

    void Mount::Cancel()
    {
        unmount(prepared ? 2 : 0);
    }

    void Mount::Close()
    {
        if (prepared) throw std::logic_error("SaveData transaction must be committed or cancelled before closing");
        unmount(0);
    }
}
