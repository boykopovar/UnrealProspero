#ifndef ANYUNREAL_RHI_COMMAND_SUBMISSION_H
#define ANYUNREAL_RHI_COMMAND_SUBMISSION_H

#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void SubmitAndWait(CommandBuffer& commands, std::uint64_t& completion, std::uint64_t value, std::uint64_t timeoutMicroseconds);
}

#endif
