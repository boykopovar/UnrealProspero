import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import zipfile

from unreal_prospero.engine import contained_path, require_engine
from unreal_prospero.process import require_tool, run


def extract(archive, destination):
    destination.mkdir(parents=True)
    if archive.name.endswith('.zip'):
        with zipfile.ZipFile(archive) as source:
            for member in source.infolist():
                target = contained_path(destination, member.filename)
                if member.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                    continue
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open(member) as input_stream, target.open('xb') as output:
                    shutil.copyfileobj(input_stream, output)
    elif archive.name.endswith('.tar.gz'):
        with tarfile.open(archive, 'r:gz') as source:
            for member in source:
                target = contained_path(destination, member.name)
                if member.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                    continue
                if not member.isfile():
                    raise ValueError(f'Unsupported package entry: {member.name}')
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.extractfile(member) as input_stream, target.open('xb') as output:
                    shutil.copyfileobj(input_stream, output)
                target.chmod(member.mode & 0o777)
    else:
        raise ValueError('Unsupported extension archive')


def native_changes(engine):
    return subprocess.check_output(['git', '-C', str(engine), 'diff', '--binary', 'HEAD', '--', 'Engine/Source', 'Engine/Build', 'Engine/Config'])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--packages', required=True, type=Path)
    parser.add_argument('--engine', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--cmake', type=Path)
    options = parser.parse_args()
    packages = sorted(path for path in options.packages.resolve(strict=True).iterdir() if path.is_file())
    if len(packages) != 1:
        raise ValueError('Expected one extension archive')
    engine = require_engine(options.engine)
    work = options.work.resolve()
    destination = work / 'relocated extension'
    if destination.exists():
        raise FileExistsError(destination)
    extract(packages[0], destination)
    before = native_changes(engine)
    cmake = options.cmake.resolve(strict=True) if options.cmake is not None else require_tool('cmake')
    command = [sys.executable, destination / 'tools/Install.py', '--engine', engine, '--cmake', cmake, '--skip-build']
    for index in range(2):
        run(command, destination, work, work / ('logs/install-' + str(index) + '.log'))
    if native_changes(engine) != before:
        raise ValueError('Installation changed native Unreal source, build or platform configuration files')
    executable = engine / 'Engine/Platforms/UnrealProspero/SDK/sdk/bin' / ('adk.exe' if os.name == 'nt' else 'adk')
    run([executable, '--version'], destination, work, work / 'logs/installed-sdk.log')
    print('Extension archive installation, reinstallation and SDK execution passed', flush=True)


if __name__ == '__main__':
    main()
