#include "UnrealProspero/File/Path.h"
#include "UnrealProspero/Text/Encoding.h"
#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace UnrealProspero::File
{
    std::basic_string_view<TCHAR> ViewPath(const TCHAR* path)
    {
        if (path == nullptr || *path == 0) throw std::invalid_argument("File path is empty");
        return path;
    }

    std::string ResolveStagedPath(std::string_view root, std::string_view path)
    {
        if (root.empty() || root.front() != '/' || root.back() != '/' || root.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid staged root");
        if (path.empty() || path.find('\0') != std::string_view::npos) throw std::invalid_argument("Invalid staged path");
        std::string value(path);
        std::replace(value.begin(), value.end(), '\\', '/');
        std::string_view relative(value);
        const bool absolute = relative.front() == '/';
        std::vector<std::string_view> components;
        const auto append = [&](std::string_view input, std::size_t floor)
        {
            while (!input.empty())
            {
                const auto separator = input.find('/');
                const auto component = input.substr(0, separator);
                input.remove_prefix(separator == std::string_view::npos ? input.size() : separator + 1);
                if (component.empty() || component == ".") continue;
                if (component == "..")
                {
                    if (components.size() <= floor) throw std::invalid_argument("Path escapes its root");
                    components.pop_back();
                }
                else components.push_back(component);
            }
        };
        if (!absolute) append(root, 0);
        const std::size_t floor = absolute ? 0 : 1;
        append(relative, floor);
        std::string result = "/";
        for (const auto component : components)
        {
            if (result.size() != 1) result.push_back('/');
            result.append(component);
        }
        if (value.back() == '/' && result.size() != 1) result.push_back('/');
        return result;
    }

    std::string EncodePath(std::basic_string_view<TCHAR> path)
    {
        if (path.empty()) throw std::invalid_argument("File path is empty");
        const auto encoded = Text::EncodeUtf8(path);
#if UNREALPROSPERO_TARGET_PS5
        return ResolveStagedPath(Text::EncodeUtf8(FUnrealProsperoPlatformProcess::BaseDir()), encoded);
#else
        return encoded;
#endif
    }

    std::string EncodePath(const TCHAR* path)
    {
        return EncodePath(ViewPath(path));
    }
}
