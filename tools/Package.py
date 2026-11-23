import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile

from unreal_prospero.engine import contained_path, versions
from unreal_prospero.install import digest, source_files
from unreal_prospero.runtime_ports import verify_distributed_files


def copy_tree(source, destination):
    source = Path(source).resolve(strict=True)
    for path in sorted(source.rglob('*')):
        if path.is_symlink():
            raise ValueError(f'Package inputs must not contain symbolic links: {path}')
        if path.is_file():
            output = destination / path.relative_to(source)
            output.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, output)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--ports', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--version', required=True)
    options = parser.parse_args()
    if re.fullmatch('[A-Za-z0-9][A-Za-z0-9._-]*', options.version) is None:
        raise ValueError('Invalid package version')
    root = Path(__file__).resolve().parents[1]
    stage = options.work.resolve() / 'package'
    if stage.exists():
        raise FileExistsError(stage)
    stage.mkdir(parents=True)
    files = source_files(root, True)
    tools = subprocess.check_output(['git', '-C', str(root), 'ls-files', 'tools', 'LICENSE', 'README.md'], text=True).splitlines()
    for name in sorted(set(files) | set(tools)):
        source = contained_path(root, name)
        destination = stage / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    ports = options.ports.resolve(strict=True)
    port_manifest = json.loads((ports / 'PortsManifest.json').read_text(encoding='utf-8'))
    if port_manifest['schemaVersion'] != 1 or not port_manifest['files']:
        raise ValueError('Invalid PS5 library manifest')
    verify_distributed_files(port_manifest['files'])
    for name, expected in port_manifest['files'].items():
        if not name.startswith('Engine/Platforms/UnrealProspero/') or name in files:
            raise ValueError(f'PS5 library file conflicts with extension source: {name}')
        source = contained_path(ports, name)
        if digest(source) != expected:
            raise ValueError(f'PS5 library hash mismatch: {name}')
        destination = contained_path(stage, name)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    sdk = options.sdk.resolve(strict=True)
    for required in ('sdk/lib/cmake/AnyDevKit/AnyDevKitConfig.cmake', 'dependencies/dependencies.json'):
        if not (sdk / required).is_file():
            raise FileNotFoundError(sdk / required)
    copy_tree(sdk, stage / 'Engine/Platforms/UnrealProspero/SDK')
    payload = stage / 'Engine/Platforms'
    files = {path.relative_to(stage).as_posix(): digest(path) for path in sorted(payload.rglob('*')) if path.is_file()}
    verify_distributed_files(files)
    manifest = {'schemaVersion': 1, 'version': options.version, 'sources': versions(), 'files': files}
    (stage / 'PackageManifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    host = 'windows' if os.name == 'nt' else 'linux'
    filename = 'UnrealProspero-' + options.version + '-' + host + '-x86_64'
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    archive = output / (filename + ('.zip' if os.name == 'nt' else '.tar.gz'))
    if os.name == 'nt':
        with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as stream:
            for path in sorted(stage.rglob('*')):
                if path.is_file():
                    stream.write(path, path.relative_to(stage).as_posix())
    else:
        with tarfile.open(archive, 'x:gz') as stream:
            for path in sorted(stage.iterdir()):
                stream.add(path, arcname=path.name)
    print(archive, flush=True)


if __name__ == '__main__':
    main()
