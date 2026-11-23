import argparse
import json
import os
from pathlib import Path
import shutil
import sys

from unreal_prospero.engine import versions
from unreal_prospero.process import require_tool, run
from unreal_prospero.sources import checkout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sources', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--cmake', type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    options = parser.parse_args()
    if options.jobs < 1:
        raise ValueError('Job count must be positive')
    sources = options.sources.resolve(strict=True)
    output = options.output.resolve()
    work = options.work.resolve()
    if output.exists():
        raise FileExistsError(f'SDK output must be a new directory: {output}')
    specification = versions()
    for name in ('anydevkit', 'anycxx'):
        checkout(specification[name], sources / name, [], work)
    cmake = str(options.cmake.resolve(strict=True)) if options.cmake is not None else require_tool('cmake')
    cc = require_tool(options.cc)
    cxx = require_tool(options.cxx)
    venv = work / 'python'
    python = venv / ('Scripts/python.exe' if os.name == 'nt' else 'bin/python')
    logs = work / 'logs'
    if not python.is_file():
        run([sys.executable, '-m', 'venv', venv], sources, work, logs / 'python.log')
    sdk_source = sources / 'anydevkit'
    run([python, '-m', 'pip', 'install', '-r', sdk_source / 'cmake/dependencies/requirements.txt'], sources, work, logs / 'python-dependencies.log')
    sdk = output / 'sdk'
    dependencies = output / 'dependencies'
    sdk_build = work / 'sdk'
    cache_options = ['-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON'] if os.environ.get('UNREALPROSPERO_CCACHE') is not None else []
    run([cmake, '-S', sdk_source, '-B', sdk_build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=' + cc, '-DCMAKE_CXX_COMPILER=' + cxx, '-DPython3_EXECUTABLE=' + str(python), '-DBUILD_TESTING=OFF', '-DANYDEVKIT_BUILD_DESKTOP_STARTUP=OFF', '-DANYDEVKIT_PACKAGE_VERSION=source-' + specification['anydevkit']['revision'][:12], *cache_options], sources, work, logs / 'sdk-configure.log')
    run([cmake, '--build', sdk_build, '--parallel', options.jobs], sources, work, logs / 'sdk-build.log')
    run([cmake, '--install', sdk_build, '--component', 'sdk', '--prefix', sdk], sources, work, logs / 'sdk-install.log')
    run([python, sdk / 'tools/setup.py', dependencies, '--target', 'ps5', '--compiler', 'clang', '--anyps5-version', specification['anyps5']['version'], '--anycxx-version', specification['anycxx']['version'], '--cache', work / 'downloads'], sources, work, logs / 'sdk-dependencies.log')
    cxx_source = sources / 'anycxx/libcxx/platform/anyps5'
    cxx_build = work / 'anycxx'
    cxx_install = work / 'anycxx-install'
    if cxx_install.exists():
        raise FileExistsError(f'C++ runtime installation must be a new directory: {cxx_install}')
    run([cmake, '-S', cxx_source, '-B', cxx_build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TOOLCHAIN_FILE=' + str(cxx_source / 'cmake/toolchains/Ps5.cmake'), '-DANYLIBCXX_SYSROOT=' + str(dependencies / 'ps5/sysroot'), '-DCMAKE_INSTALL_PREFIX=' + str(cxx_install), '-DANYLIBCXX_PACKAGE_VERSION=' + specification['anycxx']['version'].removeprefix('v')], sources, work, logs / 'anycxx-configure.log')
    run([cmake, '--build', cxx_build, '--target', 'cxx_static', '--parallel', options.jobs], sources, work, logs / 'anycxx-build.log')
    for component in ('cxx-headers', 'anylibcxx'):
        run([cmake, '--install', cxx_build, '--component', component], sources, work, logs / ('anycxx-install-' + component + '.log'))
    target = dependencies / 'ps5/anycxx'
    if target.is_symlink() or target.resolve().parent != (dependencies / 'ps5').resolve():
        raise ValueError('C++ runtime output is outside the dependency directory')
    shutil.rmtree(target)
    shutil.copytree(cxx_install, target)
    metadata_path = dependencies / 'dependencies.json'
    metadata = json.loads(metadata_path.read_text(encoding='utf-8'))
    metadata['sourceRevisions'] = {'anycxx': specification['anycxx']['revision']}
    metadata_path.write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
    print(output, flush=True)


if __name__ == '__main__':
    main()
