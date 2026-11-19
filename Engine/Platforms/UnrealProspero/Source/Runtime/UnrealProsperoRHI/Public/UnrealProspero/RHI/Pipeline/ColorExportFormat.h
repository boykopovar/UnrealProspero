#ifndef UNREALPROSPERO_RHI_COLOR_EXPORT_FORMAT_H
#define UNREALPROSPERO_RHI_COLOR_EXPORT_FORMAT_H

#include "PixelFormat.h"
#include <cstdint>
#include <span>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API std::uint32_t ResolveColorExportFormat(std::uint32_t exports, std::span<const EPixelFormat> formats);
}

#endif
