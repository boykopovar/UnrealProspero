import argparse
import json
import os
from pathlib import Path

from unreal_prospero.build import build_host, build_managed, require_dotnet
from unreal_prospero.engine import require_engine
from unreal_prospero.install import configure_sdk, install_files, source_files
from unreal_prospero.process import require_tool
from unreal_prospero.runtime_ports import prepare_runtime_ports, verify_distributed_files


def require_sdk(sdk, dependencies):
    for path in (sdk / 'bin' / ('adk.exe' if os.name == 'nt' else 'adk'), sdk / 'lib/cmake/AnyDevKit/AnyDevKitConfig.cmake', dependencies / 'include/anyps5/prx/libc/libc.hpp', dependencies / 'ps5/anycxx/lib/cmake/AnyLibcxx/AnyLibcxxConfig.cmake'):
        if not path.is_file():
            raise FileNotFoundError(path)
    metadata = json.loads((dependencies / 'dependencies.json').read_text(encoding='utf-8'))
    if metadata['schemaVersion'] != 1 or 'ps5' not in metadata['targets']:
        raise ValueError('The dependency directory does not provide PS5')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--deps', type=Path)
    parser.add_argument('--cmake', type=Path)
    parser.add_argument('--dotnet', type=Path)
    parser.add_argument('--work', type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--source', action='store_true')
    parser.add_argument('--skip-build', action='store_true', help='Skip editor and managed tool builds; prepare UE runtime components')
    options = parser.parse_args()
    if options.jobs < 1:
        raise ValueError('Job count must be positive')
    if (options.sdk is None) != (options.deps is None):
        raise ValueError('Provide both --sdk and --deps')
    root = Path(__file__).resolve().parents[1]
    engine = require_engine(options.engine)
    bundled = Path('Engine/Platforms/UnrealProspero/SDK')
    sdk = options.sdk.resolve(strict=True) if options.sdk is not None else root / bundled / 'sdk'
    dependencies = options.deps.resolve(strict=True) if options.deps is not None else root / bundled / 'dependencies'
    require_sdk(sdk, dependencies)
    cmake = options.cmake.resolve(strict=True) if options.cmake is not None else Path(require_tool('cmake'))
    dotnet = None if options.skip_build else require_dotnet(engine, options.dotnet)
    work = options.work.resolve() if options.work is not None else engine / 'Engine/Intermediate/UnrealProspero'
    files = source_files(root, options.source)
    verify_distributed_files(files)
    if options.sdk is not None:
        files = {name: digest for name, digest in files.items() if not Path(name).is_relative_to(bundled)}
    runtime_sources = prepare_runtime_ports(engine, sdk, dependencies, work, cmake, options.jobs)
    install_files(root, engine, files, runtime_sources)
    if options.sdk is None:
        sdk = engine / bundled / 'sdk'
        dependencies = engine / bundled / 'dependencies'
    configure_sdk(engine, sdk, dependencies, cmake)
    if not options.skip_build:
        build_managed(engine, dotnet, work)
        build_host(engine, dotnet, work, options.jobs)
    print('Installed UnrealProspero for Unreal Engine 5.8.0' + ('; editor and managed tool builds skipped' if options.skip_build else ''), flush=True)


if __name__ == '__main__':
    main()
