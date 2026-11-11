    const IShaderFormat* pipelineFormat = FindShaderFormat(PipelineJob->StageJobs[0]->Input.ShaderFormat, ShaderFormats);
    if (pipelineFormat && pipelineFormat->GetPipelineCompiler())
    {
        FInternalShaderCompilerFunctions::CompilePipelineInternal(*pipelineFormat->GetPipelineCompiler(), *PipelineJob, CompileCount);
        return;
    }

