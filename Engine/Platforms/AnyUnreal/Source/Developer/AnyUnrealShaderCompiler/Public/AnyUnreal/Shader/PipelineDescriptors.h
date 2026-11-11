#ifndef ANYUNREAL_SHADER_PIPELINE_DESCRIPTORS_H
#define ANYUNREAL_SHADER_PIPELINE_DESCRIPTORS_H

#include "AnyUnreal/Shader/DescriptorLayout.h"

namespace AnyUnreal::Shader
{
    struct PipelineStageDescriptors
    {
        std::vector<std::uint32_t>& Words;
        std::vector<DescriptorBinding>& Descriptors;
        std::string EntryPoint;
    };

    ANYUNREALSHADERCOMPILER_API std::vector<DescriptorBinding> MergePipelineDescriptors(std::span<const PipelineStageDescriptors> stages);
}

#endif
