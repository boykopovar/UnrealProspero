    static void CompilePipelineInternal(const IShaderPipelineCompiler& compiler, FShaderPipelineCompileJob& pipeline, int32* compileCount)
    {
        const double start = FPlatformTime::Seconds();
        pipeline.bSucceeded = false;
        TArray<FShaderCompilerInput> inputs;
        TArray<FShaderPipelineStage> stages;
        inputs.Reserve(pipeline.StageJobs.Num());
        stages.Reserve(pipeline.StageJobs.Num());
        bool ready = true;
        for (const auto& stage : pipeline.StageJobs)
        {
            auto& job = *stage;
            job.bSucceeded = false;
            job.Output.bSucceeded = false;
            job.Output.Errors.Append(job.PreprocessOutput.Errors);
            if (!job.PreprocessOutput.GetSucceeded() || job.SecondaryPreprocessOutput.IsValid() || job.SecondaryOutput.IsValid())
            {
                job.Output.AddErrorf(TEXT("Joint pipeline compilation requires successfully preprocessed single outputs."));
                ready = false;
            }
            inputs.Add(job.Input);
            if (IsValidRef(job.Input.SharedEnvironment)) inputs.Last().Environment.Merge(*job.Input.SharedEnvironment);
            job.PreprocessOutput.DecompressCode();
            stages.Add({&inputs.Last(), &job.PreprocessOutput, &job.Output});
        }
        if (ready) compiler.CompilePreprocessedShaderPipeline(stages);
        bool succeeded = ready;
        for (const auto& stage : pipeline.StageJobs) succeeded &= stage->Output.bSucceeded;
        for (const auto& stage : pipeline.StageJobs)
        {
            auto& job = *stage;
            job.Output.bSucceeded = succeeded;
            FinalizeCompiledShader(job);
            job.bSucceeded = job.Output.bSucceeded;
            job.Output.CompileTime = FPlatformTime::Seconds() - start;
            if (compileCount) ++(*compileCount);
        }
        pipeline.bSucceeded = succeeded;
    }

