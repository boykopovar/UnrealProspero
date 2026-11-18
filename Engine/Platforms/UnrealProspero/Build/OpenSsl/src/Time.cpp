#include "prx/libc/include/Api.hpp"
#include <cstdint>
#include <ctime>

extern "C" std::tm* UnrealProsperoOpenSslGmtime(const std::time_t* timer, std::tm* output)
{
    const auto seconds = static_cast<std::int64_t>(*timer);
    GuestTm converted{};
    if (!gmtime_s_nid_postfix(&seconds, &converted)) return nullptr;
    *output = {};
    output->tm_sec = converted.tm_sec;
    output->tm_min = converted.tm_min;
    output->tm_hour = converted.tm_hour;
    output->tm_mday = converted.tm_mday;
    output->tm_mon = converted.tm_mon;
    output->tm_year = converted.tm_year;
    output->tm_wday = converted.tm_wday;
    output->tm_yday = converted.tm_yday;
    output->tm_isdst = converted.tm_isdst;
    output->tm_gmtoff = converted.tm_gmtoff;
    output->tm_zone = converted.tm_zone;
    return output;
}
