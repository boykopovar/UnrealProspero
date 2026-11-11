#include "UnrealProspero/Process/Arguments.h"
#include <array>
#include <stdexcept>

bool CheckArguments()
{
    using UnrealProspero::Process::Arguments;
    if (!Arguments::Decode(0, nullptr).empty()) return false;
    for (const int count : {-1, 1})
    {
        try
        {
            Arguments::Decode(count, nullptr);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    for (const char* invalid : {static_cast<const char*>(nullptr), "\xc0\xaf", "\xed\xa0\x80"})
    {
        try
        {
            Arguments::Decode(1, &invalid);
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    const auto arguments = Arguments::Read();
    return arguments.size() == 1 && !arguments.front().empty();
}

int RunArgumentProbe()
{
    const auto arguments = UnrealProspero::Process::Arguments::Read();
    const std::array expected{TEXT("--arguments-probe"), TEXT("-name=two words"), TEXT("\"quoted\""), TEXT("path\\"), TEXT("\u0411\u6587\U0001f642"), TEXT("")};
    if (arguments.size() != expected.size() + 1) return 51;
    for (std::size_t index = 0; index != expected.size(); ++index)
    {
        if (arguments[index + 1] != expected[index]) return static_cast<int>(52 + index);
    }
    return 0;
}
