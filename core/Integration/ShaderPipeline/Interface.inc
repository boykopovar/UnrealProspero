struct FShaderPipelineStage
{
    const FShaderCompilerInput* Input;
    const FShaderPreprocessOutput* Preprocessed;
    FShaderCompilerOutput* Output;
};

class IShaderPipelineCompiler
{
public:
    virtual ~IShaderPipelineCompiler() = default;
    virtual void CompilePreprocessedShaderPipeline(TConstArrayView<FShaderPipelineStage> stages) const = 0;
};

