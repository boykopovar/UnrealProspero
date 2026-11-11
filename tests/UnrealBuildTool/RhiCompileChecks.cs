using System;
using System.IO;
using System.Linq;
using EpicGames.Core;
using UnrealBuildBase;
using UnrealBuildTool;

internal static class RhiCompileChecks
{
    public static int Add(AnyUnrealToolChain toolchain, CppCompileEnvironment platformCompile, DirectoryReference intermediate, TargetMakefileBuilder graph)
    {
        CppCompileEnvironment compile = new(platformCompile);
        string runtime = Path.Combine(Unreal.EngineDirectory.FullName, "Platforms/AnyUnreal/Source/Runtime");
        string[] modules = { "AnyUnrealShaderCommon", "AnyUnrealRHI", "ApplicationCore" };
        foreach (string module in modules) compile.UserIncludePaths.Add(new DirectoryReference(Path.Combine(runtime, module, "Public")));
        foreach (string module in new[] { "RHI", "RHICore", "RenderCore", "ApplicationCore" }) compile.SystemIncludePaths.Add(DirectoryReference.Combine(Unreal.EngineDirectory, "Source/Runtime", module, "Public"));
        compile.Definitions.AddRange(new[] { "ANYUNREALSHADERCOMMON_API=", "ANYUNREALRHI_API=", "RHI_API=", "RHICORE_API=", "RENDERCORE_API=", "APPLICATIONCORE_API=" });
        int count = 0;
        foreach (string module in modules)
        {
            FileItem[] sources = Directory.GetFiles(Path.Combine(runtime, module, "Private"), "*.cpp", SearchOption.AllDirectories).OrderBy(path => path, StringComparer.Ordinal).Select(FileItem.GetItemByPath).ToArray();
            if (sources.Length == 0) throw new InvalidDataException("RHI module contains no source files: " + module);
            if (sources.Select(source => Path.GetFileName(source.AbsolutePath)).Distinct(StringComparer.OrdinalIgnoreCase).Count() != sources.Length) throw new InvalidDataException("RHI source filenames would collide in UBT object outputs: " + module);
            CppCompileEnvironment moduleCompile = new(compile);
            moduleCompile.Definitions.Add("UE_MODULE_NAME=\"" + module + "\"");
            CPPOutput output = toolchain.CompileAllCPPFiles(moduleCompile, sources, DirectoryReference.Combine(intermediate, "rhi", module), module, graph);
            if (output.ObjectFiles.Count != sources.Length) throw new InvalidDataException("RHI source files were omitted from compilation");
            count += sources.Length;
        }
        return count;
    }
}
