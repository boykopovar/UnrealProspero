#include "UnrealProspero/File/DirectoryVisitor.h"
#include "UnrealProspero/File/Metadata.h"
#include "UnrealProspero/Kernel/Directory.h"
#include "UnrealProspero/Kernel/Error.h"
#include "UnrealProspero/Kernel/FileSystem.h"
#include "UnrealProspero/Text/Encoding.h"
#include "prx/libkernel/KernelErrors.hpp"
#include <type_traits>

namespace UnrealProspero::File
{
    namespace
    {
        template<typename TVisitor>
        bool iterate(std::basic_string_view<TCHAR> path, TVisitor& visitor)
        {
            auto encoded = Text::EncodeUtf8(path);
            std::optional<Kernel::Directory> directory;
            try
            {
                directory.emplace(encoded);
            }
            catch (const Kernel::Error& error)
            {
                if (error.Code() == SCE_KERNEL_ERROR_ENOENT || error.Code() == SCE_KERNEL_ERROR_ENOTDIR) return false;
                throw;
            }
            if (encoded.back() != '/') encoded.push_back('/');
            auto prefix = Text::DecodeUtf8(encoded);
            while (const auto entry = directory->Next())
            {
                const std::string_view leaf(entry->name, entry->nameLength);
                const auto name = Text::DecodeUtf8(leaf);
                if (!visitor.ShouldVisitLeafPathname(FStringView(name.data(), static_cast<int32>(name.size())))) continue;
                const auto status = Kernel::FileSystem::Stat(encoded + std::string(leaf));
                if (!status) throw std::runtime_error("Directory entry disappeared during iteration");
                const auto fullPath = prefix + name;
                if constexpr (std::is_same_v<TVisitor, IPlatformFile::FDirectoryStatVisitor>)
                {
                    if (!visitor.Visit(fullPath.c_str(), Metadata::FromStat(*status))) return false;
                }
                else
                {
                    if (!visitor.Visit(fullPath.c_str(), Kernel::FileSystem::IsDirectory(*status))) return false;
                }
            }
            return true;
        }
    }

    bool DirectoryVisitor::Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryVisitor& visitor)
    {
        return iterate(path, visitor);
    }

    bool DirectoryVisitor::Iterate(std::basic_string_view<TCHAR> path, IPlatformFile::FDirectoryStatVisitor& visitor)
    {
        return iterate(path, visitor);
    }
}
