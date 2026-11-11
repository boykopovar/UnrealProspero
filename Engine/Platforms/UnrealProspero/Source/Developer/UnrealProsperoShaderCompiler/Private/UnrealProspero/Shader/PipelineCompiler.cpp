#include "UnrealProspero/Shader/PipelineCompiler.h"
#include "ShaderCompilerCore.h"
#include <stdexcept>

namespace UnrealProspero::Shader
{
    void CompileShaderPipeline(std::span<const ShaderCompilationStage> stages, AgcTarget target, const FString& executable, const FString& workingRoot)
    {
        if (stages.empty()) throw std::invalid_argument("Shader pipeline has no stages");
        for (const auto& stage : stages) stage.Output.bSucceeded = false;
        try
        {
            const ShaderCompilationStage* vertex = nullptr;
            const ShaderCompilationStage* geometry = nullptr;
            const ShaderCompilationStage* pixel = nullptr;
            for (std::size_t index = 0; index < stages.size(); ++index)
            {
                const auto& stage = stages[index];
                for (std::size_t previous = 0; previous < index; ++previous)
                {
                    if (&stages[previous].Output == &stage.Output) throw std::invalid_argument("Pipeline stages share an output");
                }
                if (stage.Input.ShaderFormat != stages.front().Input.ShaderFormat || stage.Input.Target.GetPlatform() != stages.front().Input.Target.GetPlatform()) throw std::invalid_argument("Pipeline stages require the same shader platform");
                const ShaderCompilationStage** slot;
                switch (stage.Input.Target.GetFrequency())
                {
                    case SF_Vertex: slot = &vertex; break;
                    case SF_Geometry: slot = &geometry; break;
                    case SF_Pixel: slot = &pixel; break;
                    default: throw std::invalid_argument("Unsupported graphics pipeline stage");
                }
                if (*slot) throw std::invalid_argument("Duplicate graphics pipeline stage");
                *slot = &stage;
            }
            if (!vertex) throw std::invalid_argument("Graphics pipeline has no vertex stage");
            if (geometry) CompileGeometryShaders(*vertex, *geometry, target, executable, workingRoot);
            else CompileShader(vertex->Input, vertex->Preprocessed, vertex->Output, target, executable, workingRoot);
            if (pixel) CompileShader(pixel->Input, pixel->Preprocessed, pixel->Output, target, executable, workingRoot);
        }
        catch (...)
        {
            for (const auto& stage : stages) stage.Output.bSucceeded = false;
            throw;
        }
    }
}
