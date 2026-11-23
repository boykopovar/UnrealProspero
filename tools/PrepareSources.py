import argparse
from pathlib import Path

from unreal_prospero.engine import versions
from unreal_prospero.process import run
from unreal_prospero.sources import checkout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--engine-only', action='store_true')
    options = parser.parse_args()
    specification = versions()
    directory = options.directory.resolve()
    work = options.work.resolve()
    checkout(specification['unreal'], directory / 'unreal', ['Engine/Source', 'Engine/Programs/UnrealHeaderTool/Config', 'Engine/Plugins', 'Engine/Platforms', 'Engine/Build', 'Engine/Config', 'Engine/Shaders', 'Templates/TP_Blank', 'Templates/TemplateResources'], work)
    if options.engine_only:
        return
    sdk = checkout(specification['anydevkit'], directory / 'anydevkit', [], work)
    run(['git', '-C', sdk, 'submodule', 'update', '--init', '--depth=1', '--filter=blob:none'], sdk, work, work / 'logs/anydevkit-submodules.log')
    checkout(specification['anycxx'], directory / 'anycxx', ['libcxx', 'libcxxabi', 'libc', 'libunwind', 'runtimes', 'cmake', 'llvm/cmake', 'llvm/include/llvm/Config', 'compiler-rt/lib/builtins'], work)


if __name__ == '__main__':
    main()
