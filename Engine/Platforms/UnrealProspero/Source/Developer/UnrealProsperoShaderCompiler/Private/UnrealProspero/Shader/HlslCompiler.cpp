#include "UnrealProspero/Shader/HlslCompiler.h"
#include "ShaderConductorContext.h"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Shader
{
    std::vector<std::uint32_t> CompileHlslToSpirv(std::string_view source, const FShaderCompilerInput& input, FShaderCompilerOutput& output)
    {
        if (source.empty() || source.find('\0') != std::string_view::npos || source.size() > static_cast<std::size_t>(std::numeric_limits<int32>::max())) throw std::invalid_argument("Invalid prepared HLSL source");
        if (input.EntryPointName.IsEmpty()) throw std::invalid_argument("Shader entry point is empty");
        CrossCompiler::FShaderConductorContext context;
        TArray<FString> arguments;
        arguments.Add(TEXT("-fspv-use-legacy-buffer-matrix-order"));
        const FAnsiStringView sourceView(source.data(), static_cast<int32>(source.size()));
        if (!context.LoadSource(sourceView, input.VirtualSourceFilePath, input.EntryPointName, input.Target.GetFrequency(), nullptr, &arguments))
        {
            context.FlushErrors(output.Errors);
            output.bSucceeded = false;
            throw std::runtime_error("ShaderConductor could not load HLSL source");
        }
        const auto& flags = input.Environment.CompilerFlags;
        CrossCompiler::FShaderConductorOptions options(flags);
        options.bEnable16bitTypes = flags.Contains(CFLAG_AllowRealTypes);
        options.bDisableOptimizations = flags.Contains(CFLAG_SkipOptimizations) || flags.Contains(CFLAG_SkipOptimizationsDXC) || (flags.Contains(CFLAG_Debug) && !flags.Contains(CFLAG_ForceOptimization));
        TArray<uint32> words;
        const bool compiled = context.CompileHlslToSpirv(options, words);
        context.FlushErrors(output.Errors);
        if (!compiled)
        {
            output.bSucceeded = false;
            throw std::runtime_error("ShaderConductor HLSL compilation failed");
        }
        if (words.Num() < 5 || words[0] != 0x07230203) throw std::runtime_error("ShaderConductor returned invalid SPIR-V");
        return {words.GetData(), words.GetData() + words.Num()};
    }
}
