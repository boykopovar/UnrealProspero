#ifndef UNREALPROSPERO_RHI_COMMAND_SAMPLE_COVERAGE_H
#define UNREALPROSPERO_RHI_COMMAND_SAMPLE_COVERAGE_H

#include "SceTypes.hpp"
#include <cstdint>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteSampleCoverage(CommandBuffer& commands, std::uint32_t samples);
}

#endif
