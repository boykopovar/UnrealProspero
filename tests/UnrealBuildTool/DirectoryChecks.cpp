#include "AnyUnreal/Kernel/Directory.h"
#include "AnyUnreal/File/DirectoryVisitor.h"
#include "AnyUnreal/Text/Encoding.h"
#include "AnyUnreal/Kernel/File.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include "AnyUnreal/Kernel/Error.h"
#include "prx/libkernel/include/Api.hpp"
#include <set>
#include <string>
#include <vector>

namespace
{
    using namespace AnyUnreal::Kernel;

    class TestDirectory
    {
    public:
        explicit TestDirectory(std::string path) : path(std::move(path))
        {
            files.reserve(64);
            FileSystem::CreateDirectory(this->path);
        }

        ~TestDirectory() noexcept(false)
        {
            for (const auto& file : files) FileSystem::RemoveFile(file);
            FileSystem::RemoveDirectory(path);
        }

        void Add(const std::string& name)
        {
            auto filePath = path + "/" + name;
            File file(filePath, FileAccess::Write, FileCreation::CreateNew);
            files.push_back(std::move(filePath));
        }

    private:
        std::string path;
        std::vector<std::string> files;
    };

    class NameVisitor final : public IPlatformFile::FDirectoryVisitor
    {
    public:
        explicit NameVisitor(bool stop) : stop(stop) {}

        bool ShouldVisitLeafPathname(FStringView name) override
        {
            return name.Len() != 0 && name.GetData()[0] != 'x';
        }

        bool Visit(const TCHAR* path, bool directory) override
        {
            if (directory) throw std::runtime_error("Unexpected directory");
            names.insert(AnyUnreal::Text::EncodeUtf8(path));
            return !stop;
        }

        const std::set<std::string>& Names() const { return names; }

    private:
        bool stop;
        std::set<std::string> names;
    };

    class StatVisitor final : public IPlatformFile::FDirectoryStatVisitor
    {
    public:
        bool Visit(const TCHAR* path, const FFileStatData& status) override
        {
            if (!status.bIsValid || status.bIsDirectory || status.FileSize != 0) throw std::runtime_error("Invalid visitor file metadata");
            names.insert(AnyUnreal::Text::EncodeUtf8(path));
            return true;
        }

        const std::set<std::string>& Names() const { return names; }

    private:
        std::set<std::string> names;
    };

    std::set<std::string> collect(Directory& directory)
    {
        std::set<std::string> names;
        while (const auto entry = directory.Next())
        {
            if (!names.emplace(entry->name, entry->nameLength).second) throw std::runtime_error("Duplicate directory entry");
        }
        if (directory.Next()) throw std::runtime_error("Directory continued after its end");
        return names;
    }
}

bool CheckDirectories()
{
    const auto root = "AnyUnreal-enumeration-" + std::to_string(getpid_nid_postfix());
    TestDirectory owned(root);
    {
        Directory empty(root);
        if (!collect(empty).empty()) return false;
    }
    std::set<std::string> expected;
    for (int index = 0; index < 48; ++index)
    {
        const auto name = std::string(100, 'x') + std::to_string(index);
        owned.Add(name);
        expected.insert(name);
    }
    const std::string unicode = "\xc3\xa9-\xe6\x96\x87";
    owned.Add(unicode);
    expected.insert(unicode);
    Directory first(root);
    Directory second(root);
    if (collect(first) != expected || collect(second) != expected) return false;
    try
    {
        Directory invalid(root + "/" + unicode);
        return false;
    }
    catch (const AnyUnreal::Kernel::Error&)
    {
    }
    const auto wideRoot = AnyUnreal::Text::DecodeUtf8(root);
    using AnyUnreal::File::DirectoryVisitor;
    NameVisitor selected(false);
    if (!DirectoryVisitor::Iterate(wideRoot, selected) || selected.Names() != std::set<std::string>{root + "/" + unicode}) return false;
    NameVisitor stopped(true);
    if (DirectoryVisitor::Iterate(wideRoot, stopped) || stopped.Names().size() != 1) return false;
    StatVisitor metadata;
    if (!DirectoryVisitor::Iterate(wideRoot, metadata)) return false;
    std::set<std::string> fullNames;
    for (const auto& name : expected) fullNames.insert(root + "/" + name);
    if (metadata.Names() != fullNames) return false;
    NameVisitor absent(false);
    if (DirectoryVisitor::Iterate(wideRoot + TEXT("/absent"), absent) || !absent.Names().empty()) return false;
    if (DirectoryVisitor::Iterate(wideRoot + TEXT("/") + AnyUnreal::Text::DecodeUtf8(unicode), absent)) return false;
    return true;
}
