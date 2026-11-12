#include "UnrealProspero/Process/ExecutablePath.h"
#include "UnrealProspero/Kernel/PosixResult.h"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace UnrealProspero::Process
{
    std::string ExecutablePath()
    {
        constexpr std::array<int, 4> query{1, 14, 12, -1};
        std::size_t size = 0;
        UnrealProspero::Kernel::CheckPosixResult(sysctl_nid_postfix(query.data(), query.size(), nullptr, &size, nullptr, 0), "executable path size");
        if (size < 2) throw std::runtime_error("Empty executable path");
        std::vector<char> path(size);
        UnrealProspero::Kernel::CheckPosixResult(sysctl_nid_postfix(query.data(), query.size(), path.data(), &size, nullptr, 0), "executable path");
        if (size != path.size() || path.back() != 0 || std::find(path.begin(), path.end() - 1, '\0') != path.end() - 1) throw std::runtime_error("Invalid executable path");
        std::string value(path.data(), path.size() - 1);
        std::replace(value.begin(), value.end(), '\\', '/');
        return value;
    }
}
