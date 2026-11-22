import json
import os
from pathlib import Path
import sys

from .process import require_tool, run


HOST_MODULES = ('BuildSettings', 'RHI', 'UnrealProsperoShaderCommon', 'UnrealProsperoShaderCompiler', 'UnrealProsperoShaderFormat', 'UnrealProsperoShaderCooker', 'UnrealProsperoTargetPlatformCommon', 'AnyPS5TargetPlatform', 'AnyPS5TargetPlatformSettings', 'AnyPS5TargetPlatformControls')


def require_dotnet(engine, executable):
    if os.name != 'nt' and sys.platform != 'linux':
        raise ValueError('The editor host must be Windows or Linux')
    if executable is None:
        host = 'win-x64/dotnet.exe' if os.name == 'nt' else 'linux-x64/dotnet'
        executable = Path(engine) / 'Engine/Binaries/ThirdParty/DotNet/10.0' / host
    elif len(Path(executable).parts) == 1:
        executable = require_tool(str(executable))
    executable = Path(executable).resolve(strict=True)
    if not executable.is_file():
        raise FileNotFoundError(executable)
    return executable


def build_managed(engine, dotnet, work):
    programs = Path(engine) / 'Engine/Source/Programs'
    child_environment = {'DOTNET_ROOT': str(dotnet.parent), 'DOTNET_MULTILEVEL_LOOKUP': '0'}
    for name, project, output, properties in (
        ('ubt', programs / 'UnrealBuildTool/UnrealBuildTool.csproj', 'UnrealBuildTool', []),
        ('uat', programs / 'AutomationTool/AutomationTool.csproj', 'AutomationTool', ['-p:AutomationToolProjectOnly=true']),
        ('automation', Path(engine) / 'Engine/Platforms/UnrealProspero/Source/Programs/AutomationTool/UnrealProspero.Automation.csproj', None, [])
    ):
        arguments = [project, '-c', 'Development', '-p:NoWarn=1591', '-v:minimal', *properties]
        run([dotnet, 'build', *arguments], engine, work, Path(work) / ('logs/' + name + '-build.log'), child_environment)
        if output is not None:
            destination = Path(engine) / 'Engine/Binaries/DotNET' / output
            run([dotnet, 'publish', *arguments, '--no-build', '--output', destination], engine, work, Path(work) / ('logs/' + name + '-publish.log'), child_environment)


def host_project(work):
    directory = Path(work) / 'host'
    directory.mkdir(parents=True, exist_ok=True)
    project = directory / 'UnrealProsperoHost.uproject'
    target = directory / 'Source/UnrealProsperoHostEditor.Target.cs'
    target.parent.mkdir(parents=True, exist_ok=True)
    project_text = json.dumps({'FileVersion': 3, 'EngineAssociation': '', 'Category': '', 'Description': ''}, indent=2) + '\n'
    target_text = 'using UnrealBuildTool;\n\npublic class UnrealProsperoHostEditorTarget : TargetRules\n{\n    public UnrealProsperoHostEditorTarget(TargetInfo target) : base(target)\n    {\n        Type = TargetType.Editor;\n        DefaultBuildSettings = BuildSettingsVersion.V7;\n        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;\n    }\n}\n'
    for path, content in ((project, project_text), (target, target_text)):
        if path.exists() and path.read_text(encoding='utf-8') != content:
            raise ValueError(f'Generated host project was modified: {path}')
        if not path.exists():
            path.write_text(content, encoding='utf-8')
    return project


def build_host(engine, dotnet, work, jobs, no_link=False):
    if os.name != 'nt' and sys.platform != 'linux':
        raise ValueError('The editor host must be Windows or Linux')
    project = host_project(work)
    ubt = Path(engine) / 'Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll'
    platform = 'Win64' if os.name == 'nt' else 'Linux'
    arguments = [dotnet, ubt, 'UnrealProsperoHostEditor', platform, 'Development', '-project=' + str(project), '-NoUBA', '-NoXGE', '-MaxParallelActions=' + str(jobs), '-UBARootDir=' + str(Path(work) / 'uba'), '-Log=' + str(Path(work) / 'logs/editor-modules.log')]
    arguments += ['-Module=' + name for name in HOST_MODULES]
    if no_link:
        arguments += ['-NoLink', '-NoPCH']
    run(arguments, engine, work, Path(work) / 'logs/editor-build.log', {'DOTNET_ROOT': str(dotnet.parent), 'DOTNET_MULTILEVEL_LOOKUP': '0'})
