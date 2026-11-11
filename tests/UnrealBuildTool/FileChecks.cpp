#include "UnrealProspero/Kernel/File.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
    template<typename TException, typename TOperation>
    bool rejects(TOperation operation)
    {
        try
        {
            operation();
        }
        catch (const TException&)
        {
            return true;
        }
        return false;
    }

    struct OwnedPath
    {
        std::string Path;
        bool Created = false;

        ~OwnedPath() noexcept(false)
        {
            if (Created) UnrealProspero::Kernel::CheckResult(sceKernelUnlink(Path.c_str()), "sceKernelUnlink");
        }
    };
}

bool CheckFiles()
{
    using UnrealProspero::Kernel::File;
    using UnrealProspero::Kernel::FileAccess;
    using UnrealProspero::Kernel::FileCreation;
    if (!rejects<std::invalid_argument>([] { File file("", FileAccess::Read); })) return false;
    if (!rejects<std::invalid_argument>([] { File file(std::string_view("bad\0path", 8), FileAccess::Read); })) return false;
    OwnedPath path{"UnrealProspero-file-" + std::to_string(getpid_nid_postfix()) + ".bin"};
    File file(path.Path, FileAccess::ReadWrite, FileCreation::CreateNew);
    path.Created = true;
    if (!rejects<std::runtime_error>([&] { File duplicate(path.Path, FileAccess::Write, FileCreation::CreateNew); })) return false;
    std::array<std::byte, 8193> source{};
    for (std::size_t index = 0; index < source.size(); ++index) source[index] = static_cast<std::byte>(index % 251);
    file.Write(source);
    if (file.Tell() != static_cast<std::int64_t>(source.size()) || file.Size() != file.Tell()) return false;
    file.Flush();
    file.Seek(0);
    std::array<std::byte, 8200> destination{};
    if (file.Read(destination) != source.size() || !std::equal(source.begin(), source.end(), destination.begin())) return false;
    if (file.Read(destination) != 0) return false;
    if (file.Read({}) != 0) return false;
    file.Seek(37);
    if (file.ReadAt(std::span(destination).first(128), 113) != 128 || file.Tell() != 37) return false;
    if (!std::equal(destination.begin(), destination.begin() + 128, source.begin() + 113)) return false;
    if (!rejects<std::invalid_argument>([&] { file.ReadAt(destination, -1); })) return false;
    if (!rejects<std::invalid_argument>([&] { file.ReadAt(destination, std::numeric_limits<std::int64_t>::max()); })) return false;
    if (!rejects<std::invalid_argument>([&] { file.Seek(-1); })) return false;
    if (!rejects<std::invalid_argument>([&] { file.SeekFromEnd(1); })) return false;
    if (!rejects<std::invalid_argument>([&] { file.Truncate(-1); })) return false;
    file.SeekFromEnd(-12);
    if (file.Read(destination) != 12 || !std::equal(destination.begin(), destination.begin() + 12, source.end() - 12)) return false;
    const auto position = file.Tell();
    file.Truncate(3);
    if (file.Size() != 3 || file.Tell() != position) return false;
    file.Truncate(10);
    if (file.ReadAt(std::span(destination).first(7), 3) != 7) return false;
    if (!std::all_of(destination.begin(), destination.begin() + 7, [](std::byte value) { return value == std::byte{0}; })) return false;
    file.Close();
    if (!rejects<std::logic_error>([&] { file.Tell(); })) return false;
    {
        File appended(path.Path, FileAccess::Write, FileCreation::Existing, true);
        appended.Seek(0);
        appended.Write(std::span(source).first(1));
        if (appended.Size() != 11) return false;
    }
    {
        File readOnly(path.Path, FileAccess::Read);
        if (readOnly.Size() != 11) return false;
        if (!rejects<std::runtime_error>([&] { readOnly.Write(source); })) return false;
    }
    {
        File truncated(path.Path, FileAccess::Write, FileCreation::TruncateExisting);
        if (truncated.Size() != 0) return false;
    }
    return true;
}
