import argparse
from pathlib import Path

from unreal_prospero.build import managed_workspace, require_dotnet
from unreal_prospero.engine import require_engine
from unreal_prospero.process import require_tool, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--dotnet', type=Path)
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--deps', type=Path)
    parser.add_argument('--cmake', type=Path)
    options = parser.parse_args()
    if (options.sdk is None) != (options.deps is None):
        raise ValueError('Provide both --sdk and --deps')
    root = Path(__file__).resolve().parents[1]
    engine = require_engine(options.engine)
    dotnet = require_dotnet(engine, options.dotnet)
    work = options.work.resolve()
    directory = managed_workspace(work)
    cases = [('AutomationTool', 'UnrealProspero.AutomationTool.Tests', ['-rootdirectory=' + str(engine)])]
    if options.sdk is not None:
        cmake = options.cmake.resolve(strict=True) if options.cmake is not None else require_tool('cmake')
        cases.append(('UnrealBuildTool', 'UnrealBuildTool.Tests', [options.sdk.resolve(strict=True), options.deps.resolve(strict=True), cmake, work / 'rhi', '-rootdirectory=' + str(engine), '--rhi-only']))
    for name, assembly, arguments in cases:
        project = root / 'tests' / name / ('UnrealProspero.' + name + '.Tests.csproj')
        destination = work / 'tests' / name
        run([dotnet, 'build', project, '-c', 'Development', '-p:UnrealRoot=' + str(engine), '-p:NoWarn=1591', '--output', destination], directory, work, work / ('logs/' + name + '-test-build.log'))
        run([dotnet, destination / (assembly + '.dll'), *arguments], root, work, work / ('logs/' + name + '-test.log'))


if __name__ == '__main__':
    main()
