import argparse
import json
from pathlib import Path
import re
import tarfile
import zipfile

from unreal_prospero.runtime_ports import verify_distributed_files


def verify(packages, tag, assets=None):
    if not tag.startswith('v') or re.fullmatch('[A-Za-z0-9][A-Za-z0-9._-]*', tag[1:]) is None:
        raise ValueError('Invalid release tag')
    version = tag[1:]
    names = {f'UnrealProspero-{version}-windows-x86_64.zip', f'UnrealProspero-{version}-linux-x86_64.tar.gz'}
    entries = list(packages.iterdir())
    if {path.name for path in entries} != names or any(not path.is_file() for path in entries):
        raise ValueError('Expected exactly the Windows and Linux release archives')
    for path in entries:
        if path.suffix == '.zip':
            with zipfile.ZipFile(path) as archive, archive.open('PackageManifest.json') as stream:
                manifest = json.load(stream)
                verify_distributed_files(archive.namelist())
        else:
            with tarfile.open(path, 'r:gz') as archive:
                verify_distributed_files(member.name for member in archive if member.isfile())
                member = archive.getmember('PackageManifest.json')
                if not member.isfile():
                    raise ValueError('The package manifest must be a regular file')
                with archive.extractfile(member) as stream:
                    manifest = json.load(stream)
        if manifest['schemaVersion'] != 1 or manifest['version'] != version or not manifest['files']:
            raise ValueError(f'Incorrect release package manifest: {path.name}')
    if assets is not None:
        published = json.loads(assets.read_text(encoding='utf-8'))
        if published['tagName'] != tag or published['isDraft'] is not True:
            raise ValueError('Expected the matching draft release')
        uploaded = published['assets']
        if len(uploaded) != len(names) or {asset['name'] for asset in uploaded} != names:
            raise ValueError('Release assets differ from the verified archives')
        for asset in uploaded:
            if asset['size'] != (packages / asset['name']).stat().st_size:
                raise ValueError(f'Incorrect uploaded asset size: {asset["name"]}')
    print(f'Verified release archives for {tag}', flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--packages', required=True, type=Path)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--assets', type=Path)
    options = parser.parse_args()
    verify(options.packages.resolve(strict=True), options.tag, options.assets)


if __name__ == '__main__':
    main()
