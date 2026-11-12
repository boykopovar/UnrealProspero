#ifndef UNREALPROSPERO_OODLE_CTYPE_H
#define UNREALPROSPERO_OODLE_CTYPE_H

#include <cstdint>

extern "C"
{
    const std::uint16_t** UnrealProsperoOodleCtype();
    const std::int32_t** UnrealProsperoOodleLowercase();
    const std::int32_t** UnrealProsperoOodleUppercase();
}

#endif
