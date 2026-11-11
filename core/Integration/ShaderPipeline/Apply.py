import argparse
from pathlib import Path


def replace_once(text, original, replacement):
    if text.count(original) != 1:
        raise RuntimeError("Unsupported Unreal source layout: " + original[:100])
    return text.replace(original, replacement, 1)


def prepare(engine):
    fragments = Path(__file__).resolve().parent
    header = engine / "Source/Developer/TargetPlatform/Public/Interfaces/IShaderFormat.h"
    source = engine / "Source/Runtime/RenderCore/Private/ShaderCore.cpp"
    originals = {path: path.read_bytes() for path in (header, source)}
    texts = {path: data.decode("utf-8-sig").replace("\r\n", "\n") for path, data in originals.items()}
    interface = fragments.joinpath("Interface.inc").read_text()
    compiler = fragments.joinpath("Compiler.inc").read_text()
    dispatch = fragments.joinpath("Dispatch.inc").read_text()
    accessor = "    virtual const IShaderPipelineCompiler* GetPipelineCompiler() const { return nullptr; }\n"
    markers = ((header, interface), (header, accessor), (source, compiler), (source, dispatch))
    present = [fragment in texts[path] for path, fragment in markers]
    if any(present):
        if not all(present) or "static void FinalizeCompiledShader(FShaderCompileJob& Job)" not in texts[source]:
            raise RuntimeError("Incomplete or different Unreal shader pipeline integration")
        return originals, originals
    patched_header = replace_once(texts[header], "class IShaderFormat\n", interface + "class IShaderFormat\n")
    patched_header = replace_once(patched_header, "class IShaderFormat\n{\npublic:\n", "class IShaderFormat\n{\npublic:\n" + accessor)
    text = texts[source]
    begin = "\t\tFinalizeShaderCompileJob(Job.Output, Job.Input);\n"
    end = "\t\tJob.Output.CompileTime = FPlatformTime::Seconds() - TimeStart;"
    if text.count(begin) != 1 or text.count(end) != 1:
        raise RuntimeError("Unsupported Unreal shader finalization layout")
    start = text.index(begin)
    stop = text.index(end, start)
    finalize = text[start:stop]
    text = text[:start] + "\t\tFinalizeCompiledShader(Job);\n\n" + text[stop:]
    declaration = "\tstatic void CompileShaderInternal(const IShaderFormat* Compiler, FShaderCompileJob& Job, FString& OutExceptionMsg, FString& OutExceptionCallstack, int32* CompileCount)"
    helper = "    static void FinalizeCompiledShader(FShaderCompileJob& Job)\n    {\n" + finalize + "    }\n\n"
    text = replace_once(text, declaration, helper + compiler + declaration)
    entry = "void CompileShaderPipeline(const TArray<const IShaderFormat*>& ShaderFormats, FShaderPipelineCompileJob* PipelineJob, int32* CompileCount)\n{\n\tcheckf(PipelineJob->StageJobs.Num() > 0, TEXT(\"Pipeline %s has zero jobs!\"), PipelineJob->Key.ShaderPipeline->GetName());\n\n"
    text = replace_once(text, entry, entry + dispatch)
    return originals, {header: patched_header.encode("utf-8"), source: text.encode("utf-8")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    options = parser.parse_args()
    original, patched = prepare(options.engine.resolve(strict=True))
    if options.check:
        if original != patched:
            raise RuntimeError("Unreal shader pipeline integration is not installed")
        return
    written = []
    try:
        for path, data in patched.items():
            if data != original[path]:
                written.append(path)
                path.write_bytes(data)
    except BaseException:
        for path in written:
            path.write_bytes(original[path])
        raise


if __name__ == "__main__":
    main()
