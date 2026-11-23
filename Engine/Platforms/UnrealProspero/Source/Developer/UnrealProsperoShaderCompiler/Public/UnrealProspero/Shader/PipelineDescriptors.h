#ifndef UNREALPROSPERO_SHADER_PIPELINE_DESCRIPTORS_H
#define UNREALPROSPERO_SHADER_PIPELINE_DESCRIPTORS_H

#include "UnrealProspero/Shader/DescriptorLayout.h"

namespace UnrealProspero::Shader
{
    struct PipelineStageDescriptors
    {
        std::vector<std::uint32_t>& Words;
        std::vector<DescriptorBinding>& Descriptors;
        std::string EntryPoint;
    };

    enum class PipelineDescriptorLayout
    {
        ReindexStages,
        PreserveFirstStage
    };

    UNREALPROSPEROSHADERCOMPILER_API std::vector<DescriptorBinding> MergePipelineDescriptors(std::span<const PipelineStageDescriptors> stages, PipelineDescriptorLayout layout = PipelineDescriptorLayout::ReindexStages);
}

#endif
