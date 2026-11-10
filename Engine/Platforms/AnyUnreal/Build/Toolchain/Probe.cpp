#include <cstddef>
#include <string>
#include <vector>
#include <prx/libScePad/libScePad.hpp>

static_assert(sizeof(void*) == 8);
static_assert(sizeof(wchar_t) == 2);

std::size_t AnyUnrealRuntimeProbe(std::size_t count) {
    std::vector<std::wstring> values(count, L"AnyUnreal");
    std::size_t result = 0;
    for (const auto& value : values) result += value.size();
    return result;
}
