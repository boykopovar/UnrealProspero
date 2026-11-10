#include "AnyUnreal/File/DirectoryVisitor.h"
#include "AnyUnreal/File/Metadata.h"
#include "AnyUnreal/Kernel/Directory.h"
#include "AnyUnreal/Kernel/Error.h"
#include "AnyUnreal/Kernel/FileSystem.h"
#include "AnyUnreal/Text/Encoding.h"
#include "prx/libkernel/KernelErrors.hpp"
#include <type_traits>

namespace AnyUnreal::File
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
